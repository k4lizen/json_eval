#pragma once

#include "json.hpp"

// Parses expressions which use JSONPath queries
// https://www.rfc-editor.org/rfc/rfc9535
// with slight differences
class JsonExpressionParser {
public:
    static Json parse(const Json& json, const std::string& expression);

private:
    JsonExpressionParser(const Json& json, const std::string& expression); 
    Json parse();
    
    char peek();
    char next();
    bool match(const char c);
    void assert_match(const char c);
    void skip();
    bool reached_end();

    Json root;
    std::string buffer;
    int current;
    int line;
};


