#include "expressions.hpp"

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
Json JsonExpressionParser::parse_func_or_path(const Json& json) {
    int start = current;

    switch(peek()){
        
    }
}

Json JsonExpressionParser::parse() {
    current = 0;
    line = 1;

    return parse_func_or_path(root);
}

