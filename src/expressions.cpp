#include "expressions.hpp"

JsonExpressionParser::JsonExpressionParser(const Json& json,
                                           const std::string& expression) {
    this->json = json;
    this->buffer = expression;
}

Json JsonExpressionParser::parse() {
    current = 0;
    line = 1;
    
    return json;
}

Json JsonExpressionParser::parse(const Json& json, const std::string& expression) {
    JsonExpressionParser jep(json, expression);
    return jep.parse();
}
