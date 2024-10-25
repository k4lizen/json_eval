#include "expressions.hpp"
#include "json.hpp"
#include "utils.hpp"
#include <cassert>
#include <charconv>
#include <cmath>

JsonExpressionParser::JsonExpressionParser(const Json& json,
                                           const std::string& expression) {
    this->root = json;
    this->buffer = expression;
}

JsonArray JsonExpressionParser::parse(const Json& json,
                                      const std::string& expression) {
    JsonExpressionParser jep(json, expression);
    return jep.parse();
}

// Advance only if the next character matches param
bool JsonExpressionParser::match(const char c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

// TODO: is this good design?
void JsonExpressionParser::assert_match(const char c) {
    assert(peek() == c);
    next();
}

// Return current character
char JsonExpressionParser::peek() {
    if (reached_end()) {
        return '\0';
    }

    return buffer[current];
}

char JsonExpressionParser::peekNext() {
    if (current + 1 >= buffer.size()) {
        return '\0';
    }

    return buffer[current + 1];
}

// TODO: make same definition in loader.cpp? valid there?
// Advance and return next character
char JsonExpressionParser::next() {
    if (current + 1 >= buffer.size()) {
        if (current < buffer.size()) {
            current++;
        }
        return '\0';
    }

    return buffer[++current];
}

bool JsonExpressionParser::reached_end() {
    return current >= buffer.size();
}

// Skip all whitespace
void JsonExpressionParser::skip() {
    // bounds checking is happening in peek() and next()
    char c = peek();
    while (is_whitespace(c)) {
        if (c == '\n') {
            line++;
        }
        c = next();
    }
}

void JsonExpressionParser::expr_err(const std::string& msg) {
    std::string res = "Json Expression Error: " + msg + '\n';
    res += buffer + '\n';
    res += pretty_error_pointer(current);
    throw JsonExprErr(res);
}

JsonArray JsonExpressionParser::parse_max(const JsonArray& nodelist) {
    exit(1);
}
JsonArray JsonExpressionParser::parse_min(const JsonArray& nodelist) {
    exit(1);
}
JsonArray JsonExpressionParser::parse_size(const JsonArray& nodelist) {
    exit(1);
}

JsonArray JsonExpressionParser::parse_func(const JsonArray& nodelist,
                                           FuncType func) {
    assert_match('(');
    switch (func) {
    case FuncType::MAX:
        return parse_max(nodelist);
    case FuncType::MIN:
        return parse_min(nodelist);
    case FuncType::SIZE:
        return parse_size(nodelist);
    default:
        assert(0); // TODO: this is ugly
    }
}

FuncType string_to_functype(std::string_view sv) {
    if (sv == "min") {
        return FuncType::MIN;
    } else if (sv == "max") {
        return FuncType::MAX;
    } else if (sv == "size") {
        return FuncType::SIZE;
    } else {
        return FuncType::INVALID;
    }
}

// TODO: deduplicate??
// If false is returned, number is undefined
bool JsonExpressionParser::match_integer(int& number) {
    const char* cbuff = buffer.c_str();
    auto [ptr, ec] =
        std::from_chars(cbuff + current, cbuff + buffer.size() - 1, number);

    if (ec == std::errc::invalid_argument) {
        // no number at that location
        return false;
    }

    if (ec == std::errc::result_out_of_range) {
        expr_err("number out of range");
    }

    // valid number!
    current = ptr - cbuff;
    return true;
}

JsonArray JsonExpressionParser::parse_index_or_expression_selector(
    const JsonArray& nodelist) {
    assert_match('[');

    // Weirdness due to the the spec extension coming from two facts:
    // 1. We allow "a" as a valid path, no need for "$.a"
    // 2. We allow expressions inside [ ]
    // Json:
    // {
    //     "0": 1,
    //     "arr": [
    //             "as index",
    //             "as expression"
    //     ]
    // }
    // Should the query: "arr[0]" return ["as index"] or ["as expression"]?
    // Luckily digits aren't allowed as the first character for dot-notation names,
    // so we always interpret digits like literals (as index) in cases like this.

    // TODO: do I want multi-indexing?
    int idx;
    if (!match_integer(idx)) {
        // possibly there is an expression here which evaluates to an integer
        // Json intj = parse_func_or_path(nodelist);

        // if (intj.get_type() == JsonType::ARRAY) {
        //     if (intj.size() != 1) {
        //         expr_err("too many elements for array representing index");
        //     }
        //     intj = intj[0];
        // }

        // if (intj.get_type() == JsonType::NUMBER) {
        //     double pidx = intj.get_number();
        //     if (std::floor(pidx) == pidx) {
        //         idx = static_cast<int>(pidx);
        //     } else {
        //         expr_err("expression used as array index is a number but not "
        //                  "an integer");
        //     }
        // } else {
        //     expr_err(
        //         "expression used as array index doesn't evaluate to a number");
        // }
    }

    // now we have a valid index
    // Following https://www.rfc-editor.org/rfc/rfc9535#name-semantics-5
    // we need to accept negative numbers as well as out of bounds
    if (idx < 0) {
        idx = nodelist.size() - idx;
    }
    // return empty array on out of bounds
    if (idx < 0 || idx >= nodelist.size()) {
        return JsonArray();
    }

    // return nodelist[idx];
}

JsonArray JsonExpressionParser::parse_name(const JsonArray& nodelist,
                                           std::string_view name) const {
    JsonArray res;
    for (auto& node : nodelist) {
        if (node.get_type() == JsonType::OBJECT && node.obj_contains(name)) {
            res.push_back(node[name]);
        }
    }
    return res;
}

bool valid_dot_name_first(unsigned char c) {
    // name-first          = ALPHA /
    //                       "_"   /
    //                       %x80-D7FF /
    //                          ; skip surrogate code points
    //                       %xE000-10FFFF
    // ALPHA               = %x41-5A / %x61-7A    ; A-Z / a-z

    // Since we are assuming our input is in UTF-8 we don't
    // need to check for surrogates.
    // We allow all UTF-8 bytes with the (c > 127) check.
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' || c > 127;
}

bool valid_dot_name_char(unsigned char c) {
    // name-char           = name-first / DIGIT
    // DIGIT               = %x30-39              ; 0-9

    return valid_dot_name_first(c) || ('0' <= c && c <= '9');
}

bool valid_dot_notation_name(std::string_view name) {
    // https://www.rfc-editor.org/rfc/rfc9535#section-2.5.1.1
    // member-name-shorthand = name-first *name-char

    if (name.empty())
        return false;

    if (!valid_dot_name_first(name[0])) {
        return false;
    }

    for (unsigned int i = 1; i < name.size(); ++i) {
        if (!valid_dot_name_char(name[i])) {
            return false;
        }
    }
    return true;
}

JsonArray
JsonExpressionParser::parse_name_selector_dotted(const JsonArray& nodelist) {
    assert_match('.');

    int start = current;
    char c = peek();
    if (!valid_dot_name_first(c)) {
        expr_err("invalid first character for dot-notation name selector");
    }
    
    while (valid_dot_name_char(c)) {
        c = next(); // bounds checking is implicit
    }

    return parse_name(nodelist,
                      std::string_view(buffer).substr(start, current - start));
}

JsonArray
JsonExpressionParser::parse_name_selector_quoted(const JsonArray& nodelist,
                                                 char quote) {
    assert_match('[');
    assert_match(quote);

    int start = current;
    char c = peek();
    while (!reached_end() && c != quote) {
        c = next();
    }

    // either the quote or the ] isn't closed
    if (reached_end()) {
        expr_err("query ended early: unterminated name selector");
    }

    std::string_view name =
        std::string_view(buffer).substr(start, current - start);
    assert_match(quote);
    if (!match(']')) {
        expr_err("unterminated name selector, expected ]");
    }
    return parse_name(nodelist, name);
}

JsonArray JsonExpressionParser::parse_selector(const JsonArray& nodelist) {
    if (nodelist.empty()) {
        return JsonArray();
    }

    // I) We have three valid selectors inside brackets:
    // 1. (single or double) quote escaped: ["some field"]; ['some field']
    //     denoting an object key
    // 2. integer: [10]
    //     denoting an array index
    // 3. expression (non-rfc extension): [a.b["c"]]
    //     which evaluates to one 1. or 2.

    // II) We also have the dot-shorthand: .something
    //     denoting an object key
    // TODO: check if ["a"]b is okay, some implementations allow it

    // No error should be produced when applying indexing to objects
    // or keying to arrays, simply output nothing.
    // See https://www.rfc-editor.org/rfc/rfc9535#name-semantics-3
    // and https://www.rfc-editor.org/rfc/rfc9535#name-semantics-5

    char p = peek();
    assert(p == '[' || p == '.');
    char pn = peekNext();
    if (p == '[') {
        // I)
        if (pn == '\'' || pn == '"') {
            // I) 1.
            return parse_name_selector_quoted(nodelist, pn);
        } else {
            // I) 2. && 3.
            return parse_index_or_expression_selector(nodelist);
        }
    } else {
        // II)
        return parse_name_selector_dotted(nodelist);
    }
}

JsonArray JsonExpressionParser::parse_path(const JsonArray& nodelist,
                                           std::string_view obj_beginning) {
    char c = peek();
    assert (c == '.' || c == '[');
    
    JsonArray res = nodelist;
    if (obj_beginning != "") {
        // We need to parse this before we continue with this->current.
        // Doing it this way is an optimization circumventing the fact that
        // we needed to figure out whether this was a function call or
        // a path expression.

        res = parse_name(res, obj_beginning);
    }

    while (!reached_end() && (c == '.' || c == '[')) {
        // TODO: a copy happens here (trust), how to optimize?
        // this->current gets advanced inside \/
        res = parse_selector(res);
        skip();
        c = peek();
    }

    return res;
}

JsonArray JsonExpressionParser::parse_func_or_path(const JsonArray& nodelist) {
    char c;
    // $ means we are for sure in a path
    if (match('$')) {
        // jsonpath-query      = root-identifier segments
        // segments            = *(S segment)
        skip();
        c = peek();
        if (c != '.' && c != '[') {
            expr_err("invalid character, expected . or [");
        }
        
        return parse_path(nodelist, "");
    }
    // Posibilities:
    // [
    //     spec segment
    // something(
    //     function call
    // something.
    // something[
    //     dot-notation name selector
    //     (without dot since it's at the beginning of the path)
    //     (this is our extension, not in the spec)
    // something<end of string>
    //     same as above
    // something+ (et al.)
    //     same as above

    // TODO:
    // something +, something [, something .
    // aren't allowed but they should be

    c = peek();
    if (c == '[') {
        return parse_path(nodelist, "");
    }

    int start = current;
    // allowed function name characters are a subset of allowed dot name characters
    if (!valid_dot_name_first(c)) {
        expr_err("invalid first character in dot-notation name selector or function name");
    }
    c = next();
    
    while (!reached_end()) {
        switch (c) {
        case '(': {
            // something( function
            std::string_view sv =
                std::string_view(buffer).substr(start, current - start);
            FuncType func = string_to_functype(sv);
            if (func == FuncType::INVALID) {
                expr_err("invalid function name '" + std::string(sv) + "'");
            }
            return parse_func(nodelist, func);
        }
        case '.':
        case '[':
            // something. or something[ path
            return parse_path(nodelist, std::string_view(buffer).substr(
                                            start, current - start));
        case '+':
        case '-':
        case '*':
        case '/':
            return parse_name(nodelist, std::string_view(buffer).substr(start, current - start));
        default:
            if (!valid_dot_name_char(c)) {
                expr_err("invalid character in dot-notation name selector or function name");
            }
        }

        c = next();
    }

    // something<end of string>
    return parse_name(nodelist, std::string_view(buffer).substr(start, current - start));
}

JsonArray JsonExpressionParser::parse(const JsonArray& nodelist) { 
    // TODO: figure out binary operators here
    skip();
    return parse_func_or_path(nodelist);
}


JsonArray JsonExpressionParser::parse() {
    current = 0;
    line = 1;

    // As per the spec
    // https://www.rfc-editor.org/rfc/rfc9535#name-json-values-as-trees-of-nod
    // we will model the result of a query as a nodelist
    JsonArray jarr;
    jarr.push_back(root);
    return parse(jarr);
}
