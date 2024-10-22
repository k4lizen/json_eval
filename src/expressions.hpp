#pragma once

#include "json.hpp"

// Parses expressions which use JSONPath queries
// https://www.rfc-editor.org/rfc/rfc9535
class JsonExpressionParser {
public:
    JsonExpressionParser(const Json& json, const std::string& expression); 
    std::string parse();
    
private:
    Json json;
    std::string expr;
};


