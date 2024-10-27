#pragma once

#include <string>

namespace k4json {

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
    bool match_number(double& number);

    virtual void syntax_err(const std::string& msg) = 0;

    std::string buffer;
    unsigned int current; // index of character being parsed
    unsigned int line;    // line currently being parsed
};

} // namespace k4json
