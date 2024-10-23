#pragma once

#include "json.hpp"

class JsonExprErr : public std::runtime_error {
public:
    explicit JsonExprErr(const std::string& msg) : std::runtime_error(msg) {}
};

enum class FuncType {
    INVALID,
    MIN,
    MAX,
    SIZE
};

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

    [[noreturn]] void expr_error(const std::string& msg);

    Json parse_func_or_path(const Json& json);
    Json parse_func(const Json& json, FuncType func);
    Json parse_path(const Json& json, std::string_view beginning);

    Json parse_max(const Json& json);
    Json parse_min(const Json& json);
    Json parse_size(const Json& json);

    Json root;
    std::string buffer;
    unsigned int current;
    unsigned int line;
};


