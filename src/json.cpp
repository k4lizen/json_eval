#include <cassert>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>

#include "json.hpp"

bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

// return a description of the location of the error
std::string Json::error_line() {
    std::string desc = "line: " + std::to_string(line)
        + " position: " + std::to_string(current) + "\n";

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
        desc += std::to_string(line - 1) + ":" + buffer.substr(prev_ln_start + 1, ln_start - prev_ln_start);
    }
    
    // return the line which caused the error
    std::string cur_line_num = std::to_string(line);
    desc += cur_line_num + ":" + buffer.substr(ln_start + 1, (ln_end - ln_start));

    // point to the exact problematic symbol
    int padding = cur_line_num.size() + 1 + static_cast<int>(current) - (ln_start + 1);
    for (int i = 0; i < padding; ++i) {
        desc += ' ';
    }
    desc += '^';
    for (int i = 0; i < 10; ++i) {
        desc += '~';
    }
    desc += '\n';

    return desc;
}

[[noreturn]] void Json::load_err(const std::string& msg) {
    std::string err_msg = "Load Error: " + msg + '\n';
    err_msg += error_line();

    throw JsonLoadErr(err_msg);
}

Json::Json(const std::string& file_name) {
    std::ifstream infile(file_name);

    if (!infile.good()) {
        infile.close();
        load_err("failed opening file \"" + file_name + "\". Does it exist?");
    }

    std::stringstream sstr;
    infile >> sstr.rdbuf();
    buffer = sstr.str();
    infile.close();

    if (buffer.empty()) {
        load_err("file \"" + file_name + "\" is empty.");
    }

    std::cout << "File contents: " << std::endl << buffer;

    // Main parsing logic
    root = load();
}

// Advance only if the next character matches param
bool Json::match(const char c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

void Json::assert_match(const char c) {
    assert(peek() == c);
    next();
}

// Return current character
char Json::peek() {
    if (reached_end()) {
        return '\0';
    }

    return buffer[current];
}

// Advance and return next character
char Json::next() {
    if (reached_end() || current + 1 >= buffer.size()) {
        return '\0';
    }

    return buffer[++current];
}

bool Json::reached_end() {
    return current >= buffer.size();
}

// Skip all whitespace
void Json::skip() {
    // bounds checking is happening in peek() and next()
    char c = peek();
    while (is_whitespace(c)) {
        if (c == '\n') {
            line++;
        }
        c = next();
    }
}

void Structure::array_add(const Structure& elem) {
    assert(tag == StructureType::ARRAY);
    std::get<StructureArray>(val).push_back(elem);
}

void Structure::obj_add(const std::string& key, const Structure& child) {
    assert(tag == StructureType::OBJECT);
    std::get<StructureMap>(val)[key] = child;
}

void Structure::obj_add(const KeyedStructure& key_val) {
    assert(tag == StructureType::OBJECT);
    std::get<StructureMap>(val)[key_val.first] = key_val.second;
}

// Consumes a hex character from the buffer and returns it
unsigned int Json::unhexbyte(){
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
unsigned int Json::parse_codepoint() {
    if (current + 3 >= buffer.size()) {
        load_err("the \\uXXXX escape needs four characters");
    }

    unsigned int codepoint = unhexbyte();
    codepoint = (codepoint << 4) | unhexbyte();
    codepoint = (codepoint << 4) | unhexbyte();
    codepoint = (codepoint << 4) | unhexbyte();
    return codepoint;
}

std::string Json::parse_unicode() {
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
        load_err("codepoint is for the low part of a utf-16 surrogate pair, but there is no preceding high part");
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

        codepoint = 0x10000 + (((codepoint - 0xD800) << 10) | (low_part - 0xDC00));
    }

    // Now that we have the actual value of the unicode codepoint being encoded,
    // we "have to" re-encode it to UTF-8 (no surrogates!) for storage
    assert(codepoint <= 0x10FFFF);
    
    std::string result;
    if (codepoint < 0x80) {
        // 1 code unit: 0xxxxxxx (ascii)
        result =  static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        // 2 code units: 110xxxxx 10xxxxxx
        result =  static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        result += static_cast<char>(0x80 | ((codepoint >> 0) & 0x3F));        
    } else if (codepoint < 0x10000){
        // 3 code units: 1110xxxx 10xxxxxx 10xxxxxx
        result =  static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        result += static_cast<char>(0x80 | ((codepoint >>  6) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >>  0) & 0x3F));
    } else {
        // 4 code units: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        result =  static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));    
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));    
        result += static_cast<char>(0x80 | ((codepoint >>  6) & 0x3F));    
        result += static_cast<char>(0x80 | ((codepoint >>  0) & 0x3F));    
    }
    
    return result;
}

std::string Json::parse_escaped() {
    // string section of https://www.json.org/json-en.html
    assert_match('\\');

    switch (peek()) {
    case '"':
        return "\"";
    case '\\':
        return "\\";
    case '/':
        return "/";
    case 'b':
        return "\b";
    case 'f':
        return "\f";
    case 'n':
        return "\n";
    case 'r':
        return "\r";
    case 't':
        return "\t";
    case 'u':
        return parse_unicode();
    default:
        load_err("invalid escape sequence");
    }
}

std::string Json::load_string() {
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
            if (static_cast<int>(c) < 0x20) {
                load_err("strings cannot include unescaped control characters");
            }

            sstream << c;
            next();
        }
    }

    load_err("unterminated string");
}

bool is_end_control(const char c) {
    return c == ',' || c == '}' || c == ']';
}

bool Json::match_false() {
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

bool Json::match_true() {
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

bool Json::match_null() {
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
bool Json::match_number(double& number) {
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

Structure Json::load_value() {
    skip();

    switch (peek()) {
    case '{':
        return load_object();
    case '[':
        return load_array();
    case '"':
        return Structure(load_string());
    default:
        if (match_true()) {
            return Structure(true);
        }
        if (match_false()) {
            return Structure(false);
        }
        if (match_null()) {
            return Structure(Literal(LiteralType::NULLVAL));
        }
        double number;
        if (match_number(number)) {
            return Structure(number);
        }

        load_err("unexpected symbol for value");
    }
}

KeyedStructure Json::load_pair() {
    assert(peek() == '"');

    std::string key = load_string();
    skip();

    if (!match(':')) {
        load_err("key string must be followed by a semicolon");
    }

    Structure val = load_value();

    return KeyedStructure(key, val);
}

Structure Json::load_object() {
    assert_match('{');

    Structure node(StructureType::OBJECT);

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

Structure Json::load_array() {
    assert_match('[');

    Structure node(StructureType::ARRAY);

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

Structure Json::load() {
    // The most outer structure is handled seperately
    // since it must be an object or array
    skip();

    switch (peek()) {
    case '{':
        return load_object();
    case '[':
        return load_array();
    default:
        load_err("json must be object or array");
    }
}

Literal::Literal() : tag(LiteralType::INVALID) {}

Literal::Literal(const LiteralType& lt) {
    tag = lt;
    val = false; // setting the memory to 0 just in case
}

Literal::Literal(const std::string& str) {
    tag = LiteralType::STRING;
    val = str;
}

Literal::Literal(const bool v) {
    tag = LiteralType::BOOL;
    val = v;
}

Literal::Literal(const double num) {
    tag = LiteralType::NUMBER;
    val = num;
}

Structure::Structure(const double num) {
    tag = StructureType::LITERAL;
    val = Literal(num);
}

Structure::Structure(const Literal& lit) {
    tag = StructureType::LITERAL;
    val = lit;
}

Structure::Structure(const std::string& str) {
    tag = StructureType::LITERAL;
    val = Literal(str);
}

Structure::Structure(const StructureType& tag) {
    this->tag = tag;
    switch (tag) {
    case StructureType::OBJECT:
        val = std::map<std::string, Structure>();
        break;
    case StructureType::ARRAY:
        val = std::vector<Structure>();
        break;
    case StructureType::LITERAL:
        val = Literal();
        break;
    case StructureType::INVALID:
        assert(0);
    }
}

Structure::Structure(const bool v) {
    tag = StructureType::LITERAL;
    val = Literal(v);
}
