#pragma once

#include "catch_amalgamated.hpp"
#include "loader.hpp"

class JsonErrorMatcher : Catch::Matchers::MatcherGenericBase {
public:
    JsonErrorMatcher(int line, int position, const std::string& err_msg);
    bool match(const JsonLoadErr& err) const;
    std::string describe() const override;
    // docs don't say this is needed but doesn't work without it?
    std::string toString() const;
private:
    std::string err_msg;
    int line;
    int position;
};

JsonErrorMatcher EqualsJError(int line, int position, const std::string& err_msg);
