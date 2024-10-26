#include "parser.hpp"
#include "utils.hpp"
#include <cassert>

// Advance only if the next character matches param
bool Parser::match(const char c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

// TODO: is this good design?
void Parser::assert_match(const char c) {
    assert(peek() == c);
    next();
}

// Return current character
char Parser::peek() {
    if (reached_end()) {
        return '\0';
    }

    return buffer[current];
}

char Parser::peekNext() {
    if (current + 1 >= buffer.size()) {
        return '\0';
    }

    return buffer[current + 1];
}

// TODO: make same definition in loader.cpp? valid there?
// Advance and return next character
char Parser::next() {
    if (current + 1 >= buffer.size()) {
        if (current < buffer.size()) {
            current++;
        }
        return '\0';
    }

    return buffer[++current];
}

bool Parser::reached_end() {
    return current >= buffer.size();
}

// Skip all whitespace
void Parser::skip() {
    // bounds checking is happening in peek() and next()
    char c = peek();
    while (is_whitespace(c)) {
        if (c == '\n') {
            line++;
        }
        c = next();
    }
}

