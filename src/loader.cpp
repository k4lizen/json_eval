#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "json.hpp"
#include "utils.hpp"
#include "loader.hpp"

bool is_end_control(const char c) {
    return c == ',' || c == '}' || c == ']';
}

// return a description of the location of the error
std::string JsonLoader::error_line() {
    std::string desc = "line: " + std::to_string(line) +
                       " position: " + std::to_string(current) + "\n";

    int ln_start, ln_end;
    ln_start = ln_end = current;
    while (ln_start >= 0 && buffer[ln_start] != '\n') {
        ln_start--;
    }
    while (ln_end < static_cast<int>(buffer.size()) && buffer[ln_end] != '\n') {
        ln_end++;
    }

    // return the line before the error, for better visibility
    if (ln_start > 0) {
        int prev_ln_start = ln_start - 1;
        while (prev_ln_start >= 0 && buffer[prev_ln_start] != '\n') {
            prev_ln_start--;
        }
        if (ln_start >= 0) {
            desc += "...\n";
        }
        desc += std::to_string(line - 1) + ":" +
                buffer.substr(prev_ln_start + 1, ln_start - prev_ln_start);
    }

    // return the line which caused the error
    std::string cur_line_num = std::to_string(line);
    desc +=
        cur_line_num + ":" + buffer.substr(ln_start + 1, (ln_end - ln_start));

    // if this is the last line of the file, it may not have a \n
    if (desc[desc.size() - 1] != '\n'){
        desc += '\n';
    }

    // point to the exact problematic symbol
    int padding =
        cur_line_num.size() + 1 + static_cast<int>(current) - (ln_start + 1);
    desc += pretty_error_pointer(padding);
    return desc;
}

[[noreturn]] void JsonLoader::load_err(const std::string& msg) {
    std::string err_msg = "Load Error: " + msg + '\n';
    err_msg += error_line();

    throw JsonLoadErr(err_msg);
}

Json JsonLoader::from_file(const std::string& file_name) {
    std::ifstream infile(file_name);

    if (!infile.good()) {
        infile.close();
        throw std::runtime_error("Failed opening file " + file_name +
                                 ". Does it exit?");
    }

    // Read the whole file into a std::string
    // bad if the file is larger than free RAM
    std::stringstream sstr;
    infile >> sstr.rdbuf();
    infile.close();
    std::string file_contents = sstr.str();

    if (file_contents.empty()) {
        throw std::runtime_error("File " + file_name + " empty.");
    }

    JsonLoader jl(file_contents);
    // Main parsing logic
    return jl.load();
}

Json JsonLoader::from_string(const std::string& str) {
    JsonLoader jl(str);
    return jl.load();
}

JsonLoader::JsonLoader(const std::string& data) {
    buffer = data;
    line = 1;
    current = 0;
}

// Advance only if the next character matches param
bool JsonLoader::match(const char c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

void JsonLoader::assert_match(const char c) {
    assert(peek() == c);
    next();
}

// Return current character
char JsonLoader::peek() {
    if (reached_end()) {
        return '\0';
    }

    return buffer[current];
}

// Advance and return next character
char JsonLoader::next() {
    if (reached_end() || current + 1 >= buffer.size()) {
        return '\0';
    }

    return buffer[++current];
}

bool JsonLoader::reached_end() {
    return current >= buffer.size();
}

// Skip all whitespace
void JsonLoader::skip() {
    // bounds checking is happening in peek() and next()
    char c = peek();
    while (is_whitespace(c)) {
        if (c == '\n') {
            line++;
        }
        c = next();
    }
}

// Consumes a hex character from the buffer and returns it
unsigned int JsonLoader::unhexbyte() {
    char c = peek();
    next();

    if ('0' <= c && c <= '9') {
        return static_cast<unsigned int>(c - '0');
    } else if ('a' <= c && c <= 'f') {
        return static_cast<unsigned int>(c - 'a' + 10);
    } else if ('A' <= c && c <= 'F') {
        return static_cast<unsigned int>(c - 'A' + 10);
    } else {
        current -= 1;
        load_err("invalid hex character in \\uXXXX escape sequence");
    }
}

// Returns a two-byte value determined by a \uXXXX escape
unsigned int JsonLoader::parse_codepoint() {
    if (current + 3 >= buffer.size()) {
        load_err("the \\uXXXX escape needs four characters");
    }

    unsigned int codepoint = unhexbyte();
    codepoint = (codepoint << 4) | unhexbyte();
    codepoint = (codepoint << 4) | unhexbyte();
    codepoint = (codepoint << 4) | unhexbyte();
    return codepoint;
}

std::string JsonLoader::parse_unicode() {
    assert_match('u');
    unsigned int codepoint = parse_codepoint();

    //  https://www.rfc-editor.org/rfc/rfc8259#section-7

    // > JSON text exchanged between systems that are not part of a closed
    // > ecosystem MUST be encoded using UTF-8 [RFC3629].

    // But also:

    // > To escape an extended character that is not in the Basic Multilingual
    // > Plane, the character is represented as a 12-character sequence,
    // > encoding the UTF-16 surrogate pair.  So, for example, a string
    // > containing only the G clef character (U+1D11E) may be represented as
    // > "\uD834\uDD1E".

    // weird design.

    // surrogate pair, low part
    if (0xDC00 <= codepoint && codepoint <= 0xDFFF) {
        load_err("codepoint is for the low part of a utf-16 surrogate pair, "
                 "but there is no preceding high part");
    }

    // surrogate pair, high part
    if (0xD800 <= codepoint && codepoint <= 0xDBFF) {
        if (current + 5 >= buffer.size() || !match('\\') || !match('u')) {
            load_err("expected second (low) part of utf-16 surrogate pair");
        }

        unsigned int low_part = parse_codepoint();
        if (!(0xDC00 <= low_part && low_part <= 0xDFFF)) {
            current -= 4;
            load_err("codepoint isn't valid low part of utf-16 surrogate pair");
        }

        codepoint =
            0x10000 + (((codepoint - 0xD800) << 10) | (low_part - 0xDC00));
    }

    // Now that we have the actual value of the unicode codepoint being encoded,
    // we "have to" re-encode it to UTF-8 (no surrogates!) for storage
    assert(codepoint <= 0x10FFFF);

    std::string result;
    if (codepoint < 0x80) {
        // 1 code unit: 0xxxxxxx (ascii)
        result = static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        // 2 code units: 110xxxxx 10xxxxxx
        result = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));
    } else if (codepoint < 0x10000) {
        // 3 code units: 1110xxxx 10xxxxxx 10xxxxxx
        result = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));
    } else {
        // 4 code units: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        result = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));
    }

    return result;
}

std::string JsonLoader::parse_escaped() {
    // string section of https://www.json.org/json-en.html
    assert_match('\\');

    switch (peek()) {
    case '"':
        next();
        return "\"";
    case '\\':
        next();
        return "\\";
    case '/':
        next();
        return "/";
    case 'b':
        next();
        return "\b";
    case 'f':
        next();
        return "\f";
    case 'n':
        next();
        return "\n";
    case 'r':
        next();
        return "\r";
    case 't':
        next();
        return "\t";
    case 'u':
        return parse_unicode();
    default:
        load_err("invalid escape sequence");
    }
}

std::string JsonLoader::load_string() {
    assert_match('"');
    // https://www.rfc-editor.org/rfc/rfc8259#section-7

    std::stringstream sstream;

    while (!reached_end()) {
        char c = peek();

        switch (c) {
        case '"':
            next();
            return sstream.str();
        case '\\':
            sstream << parse_escaped();
            break;
        default:
            if (static_cast<unsigned int>(c) < 0x20) {
                load_err("strings cannot include unescaped control characters");
            }

            sstream << c;
            next();
        }
    }

    load_err("unterminated string");
}

bool JsonLoader::match_false() {
    if (current + 5 < buffer.size() && buffer[current] == 'f' &&
        buffer[current + 1] == 'a' && buffer[current + 2] == 'l' &&
        buffer[current + 3] == 's' && buffer[current + 4] == 'e' &&
        // need to make sure it isn't something like falseatswa, using whitelist
        (is_end_control(buffer[current + 5]) ||
         is_whitespace(buffer[current + 5]))) {

        current += 5;
        return true;
    }

    return false;
}

bool JsonLoader::match_true() {
    if (current + 4 < buffer.size() && buffer[current] == 't' &&
        buffer[current + 1] == 'r' && buffer[current + 2] == 'u' &&
        buffer[current + 3] == 'e' &&
        // need to make sure it isn't something like trueatswa, using whitelist
        (is_end_control(buffer[current + 4]) ||
         is_whitespace(buffer[current + 4]))) {

        current += 4;
        return true;
    }

    return false;
}

bool JsonLoader::match_null() {
    if (current + 4 < buffer.size() && buffer[current] == 'n' &&
        buffer[current + 1] == 'u' && buffer[current + 2] == 'l' &&
        buffer[current + 3] == 'l' &&
        // need to make sure it isn't something like nullatswa, using whitelist
        (is_end_control(buffer[current + 4]) ||
         is_whitespace(buffer[current + 4]))) {

        current += 4;
        return true;
    }

    return false;
}

// If false is returned, number is undefined
bool JsonLoader::match_number(double& number) {
    // https://www.rfc-editor.org/rfc/rfc8259#section-6

    // > Since software that implements
    // > IEEE 754 binary64 (double precision) numbers [IEEE754] is generally
    // > available and widely used, good interoperability can be achieved by
    // > implementations that expect no more precision or range than these
    // > provide, [...]
    //
    // So we will use a double (64 bits) for the number type

    // We can't use strtod and friends for the conversion because they
    // don't conform to the json definition of a number, but from_chars
    // does. Also, see Notes section in
    // https://en.cppreference.com/w/cpp/utility/from_chars

    const char* cbuff = buffer.c_str();
    // We give it the whole rest of the buffer, it will only care about the
    // initial valid part
    auto [ptr, ec] =
        std::from_chars(cbuff + current, cbuff + buffer.size() - 1, number);

    if (ec == std::errc::invalid_argument) {
        // no number at that location
        return false;
    }

    if (ec == std::errc::result_out_of_range) {
        load_err("number out of range");
    }

    // valid number!
    current = ptr - cbuff;
    return true;
}

Json JsonLoader::load_value() {
    skip();

    switch (peek()) {
    case '{':
        return load_object();
    case '[':
        return load_array();
    case '"':
        return Json(load_string());
    default:
        if (match_true()) {
            return Json(true);
        }
        if (match_false()) {
            return Json(false);
        }
        if (match_null()) {
            return Json();
        }
        double number;
        if (match_number(number)) {
            return Json(number);
        }

        load_err("unexpected symbol for value");
    }
}

KeyedJson JsonLoader::load_pair() {
    assert(peek() == '"');

    const std::string key = load_string();
    skip();

    if (!match(':')) {
        load_err("key string must be followed by a semicolon");
    }

    return KeyedJson(key, load_value());
}

Json JsonLoader::load_object() {
    assert_match('{');

    Json node((JsonMap()));

    // empty object
    skip();
    if (match('}')) {
        return node;
    }

    // whether we are expecting another item in the object
    bool pending = true;

    while (!reached_end()) {
        skip();

        if (pending) {
            if (peek() == '\"') {
                node.obj_add(load_pair());
                pending = false;
                continue;
            } else {
                load_err("unexpected symbol, wanted key-value pair");
            }
        }

        // not currently pending
        switch (peek()) {
        case ',':
            pending = true;
            next();
            break;
        case '}':
            next();
            return node;
        default:
            load_err("unexpected symbol, wanted , or }");
        }
    }

    load_err("reached EOF without closing curly brace");
}

Json JsonLoader::load_array() {
    assert_match('[');

    Json node((JsonArray()));

    // empty array
    skip();
    if (match(']')) {
        return node;
    }

    // whether we are expecting another item in the array
    bool pending = true;

    while (!reached_end()) {
        skip();

        if (pending) {
            node.array_add(load_value());
            pending = false;
            continue;
        }

        // not currently pending
        switch (peek()) {
        case ']':
            next();
            return node;
        case ',':
            pending = true;
            next();
            break;
        default:
            load_err("unexpected symbol, wanted , or ]");
        }
    }

    load_err("reached EOF without closing square brace");
}

// Initiates the parsing logic of JsonLoader
// If strict is true only objects and arrays are accepted
// as valid JSON. (default=true)
Json JsonLoader::load(bool strict) {
    // The JSON RFC allows both for the stricter and more
    // lax definition.
    // https://www.rfc-editor.org/rfc/rfc8259

    if (strict) {
        skip();

        switch (peek()) {
        case '{':
            return load_object();
        case '[':
            return load_array();
        default:
            load_err("json must be object or array");
        }
    } else {
        return load_value();
    }
}
