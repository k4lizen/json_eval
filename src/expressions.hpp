#pragma once

#include "json.hpp"
#include "generic_parser.hpp"

namespace k4json {

class ExprSyntaxErr : public std::runtime_error {
public:
    explicit ExprSyntaxErr(const std::string& msg) : std::runtime_error(msg) {}
};

class ExprValueErr : public std::runtime_error {
public:
    explicit ExprValueErr(const std::string& msg) : std::runtime_error(msg) {}
};

enum class FuncType {
    INVALID,
    MIN,
    MAX,
    SIZE
};

enum class Operator {
    NONE,
    PLUS,
    MINUS,
    MUL,
    DIV
};

// Parses expressions which use JSONPath queries
// https://www.rfc-editor.org/rfc/rfc9535
// with slight differences
class JsonExpressionParser : private Parser {
public:
    static JsonArray parse(const Json& json, const std::string& expression);

private:
    JsonExpressionParser(const Json& json, const std::string& expression); 
    JsonArray parse();
    JsonArray parse_inner();

    [[noreturn]] void syntax_err(const std::string& msg) override;
    [[noreturn]] void value_err(const std::string& msg);

    JsonArray parse_func_or_path();
    JsonArray parse_func(FuncType func);
    JsonArray parse_path(std::string_view obj_beginning);

    JsonArray parse_name(const JsonArray& nodelist, std::string_view name) const;
    JsonArray parse_name_selector_quoted(const JsonArray& nodelist, char quote);
    JsonArray parse_name_selector_dotted(const JsonArray& nodelist);
    JsonArray parse_expr_selector(const JsonArray& nodelist);
    JsonArray parse_selector(const JsonArray& nodelist);

    JsonArray evaluate_function(FuncType func, std::vector<Json>& arguments);
    JsonArray evaluate_max(std::vector<Json>& arguments);
    JsonArray evaluate_min(std::vector<Json>& arguments);
    JsonArray evaluate_size(std::vector<Json>& arguments);

    JsonArray rootlist;
};

JsonArray parse(const Json& json, const std::string& expression);

} // namespace k4json
