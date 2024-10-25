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
    static JsonArray parse(const Json& json, const std::string& expression);

private:
    JsonExpressionParser(const Json& json, const std::string& expression); 
    JsonArray parse();
    JsonArray parse_inner();

    // TODO: can i somehow deduplicate all this
    char peek();
    char peekNext();
    char next();
    bool match(const char c);
    void assert_match(const char c);
    void skip();
    bool reached_end();

    [[noreturn]] void expr_err(const std::string& msg);

    JsonArray parse_func_or_path(const JsonArray& nodelist);
    JsonArray parse_func(const JsonArray& nodelist, FuncType func);
    JsonArray parse_path(const JsonArray& nodelist, std::string_view obj_beginning);

    bool match_integer(int& number);
    JsonArray parse_name(const JsonArray& nodelist, std::string_view name) const;
    JsonArray parse_name_selector_quoted(const JsonArray& nodelist, char quote);
    JsonArray parse_name_selector_dotted(const JsonArray& nodelist);
    JsonArray parse_expr_selector(const JsonArray& nodelist);
    JsonArray parse_selector(const JsonArray& nodelist);

    JsonArray parse_max(const JsonArray& nodelist);
    JsonArray parse_min(const JsonArray& nodelist);
    JsonArray parse_size(const JsonArray& nodelist);

    JsonArray rootlist;
    std::string buffer;
    unsigned int current;
    unsigned int line;
};


