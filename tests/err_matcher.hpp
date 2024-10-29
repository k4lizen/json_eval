#pragma once

#include "catch_amalgamated.hpp"
#include "expressions.hpp"
#include "loader.hpp"

class JsonErrorMatcher : Catch::Matchers::MatcherGenericBase {
public:
    JsonErrorMatcher(int line, int position, const std::string& err_msg);
    JsonErrorMatcher(int position, const std::string& err_msg);
    JsonErrorMatcher(const std::string& err_msg);
    bool match(const k4json::JsonLoadErr& err) const;
    bool match(const k4json::JsonTypeErr& err) const;
    bool match(const k4json::ExprSyntaxErr& err) const;
    bool match(const k4json::ExprValueErr& err) const;
    std::string describe() const override;
    // docs don't say this is needed but doesn't work without it?
    std::string toString() const;

private:
    std::string err_msg;
    int line;
    int position;
};

JsonErrorMatcher EqualsJError(int line, int position,
                              const std::string& err_msg);
JsonErrorMatcher EqualsJError(int position, const std::string& err_msg);
JsonErrorMatcher EqualsJError(const std::string& err_msg);
