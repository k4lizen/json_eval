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
    
    Json json;
    std::string buffer;
    int current;
    int line;
};


