#include "expressions.hpp"

JsonExpressionParser::JsonExpressionParser(const Json& json, const std::string& expression) {
    this->json = json;
    this->expr = expression;
}    

std::string JsonExpressionParser::parse() {
    // probably need to reset internals here as well
    
    return "happily parsed";
}
