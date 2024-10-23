#include "expressions.hpp"
#include "json.hpp"
#include "utils.hpp"
#include <cassert>
#include <charconv>

JsonExpressionParser::JsonExpressionParser(const Json& json,
                                           const std::string& expression) {
    this->root = json;
    this->buffer = expression;
}

Json JsonExpressionParser::parse(const Json& json, const std::string& expression) {
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

// Advance and return next character
char JsonExpressionParser::next() {
    if (reached_end() || current + 1 >= buffer.size()) {
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

Json JsonExpressionParser::parse_max(const Json& json) {
    exit(1);
}
Json JsonExpressionParser::parse_min(const Json& json) {
    exit(1);
}
Json JsonExpressionParser::parse_size(const Json& json) {
    exit(1);
}

Json JsonExpressionParser::parse_func(const Json& json, FuncType func) {
    assert_match('(');
    switch(func) {
    case FuncType::MAX:
        return parse_max(json);
    case FuncType::MIN:
        return parse_min(json);
    case FuncType::SIZE:
        return parse_size(json);
    default:
        assert(0); // TODO: this is ugly, but what do
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
bool JsonExpressionParser::match_number(double& number) {
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

Json JsonExpressionParser::parse_selector_obj(const Json& json, std::string_view selector) {

}

Json JsonExpressionParser::parse_selector_array(const Json& json, std::string_view selector) {
    
}


Json JsonExpressionParser::parse_selector(const Json& json, std::string_view selector) {
    switch(json.get_type()) {
    case JsonType::OBJECT:
        return parse_selector_obj(json, selector);
    case JsonType::ARRAY:
        return parse_selector_array(json, selector);
    case JsonType::NUMBER:
    case JsonType::STRING:
    case JsonType::BOOL:
    case JsonType::NULLVAL:
        return Json(JsonArray()); // empty array: [ ]
    case JsonType::INVALID:
        assert(0); 
   }
}

Json JsonExpressionParser::parse_path(const Json& json, std::string_view beginning) {
    Json res = parse_selector(beginning);
}

Json JsonExpressionParser::parse_func_or_path(const Json& json) {
    int start = current;

    char c = peek();
    while(!reached_end()) {
        switch(c) {
        case '(': {
            // function call (min, max, size etc...)
            std::string_view sv = std::string_view(buffer).substr(start, current - start);
            FuncType func = string_to_functype(sv);
            if (func == FuncType::INVALID) {
                expr_err("invalid function name '" + std::string(sv) + "'");
            }
            return parse_func(json, func);
        }
        case '.':
        case '[':
            // now we know we are in a jsonpath
            return parse_path(json, std::string_view(buffer).substr(start, current - start));
        default:
            break;
        }

        c = next();
    }

    return Json();
}

Json JsonExpressionParser::parse() {
    current = 0;
    line = 1;

    return parse_func_or_path(root);
}

