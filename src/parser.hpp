#pragma once

#include <string>

// Generic parser implementation
class Parser {
protected:
    char peek();
    char peekNext();
    char next();
    bool match(const char c);
    void assert_match(const char c);
    void skip();
    bool reached_end();

    std::string buffer;
    unsigned int current; // index of character being parsed
    unsigned int line;    // line currently being parsed
};
