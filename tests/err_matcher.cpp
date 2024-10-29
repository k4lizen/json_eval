#include "err_matcher.hpp"
#include "expressions.hpp"

#include <format>

JsonErrorMatcher::JsonErrorMatcher(int line, int position,
                                   const std::string& err_msg) {
    this->line = line;
    this->position = position;
    this->err_msg = err_msg;
}

JsonErrorMatcher::JsonErrorMatcher(const std::string& err_msg) {
    this->line = -1;
    this->position = -1;
    this->err_msg = err_msg;
}

JsonErrorMatcher::JsonErrorMatcher(int position, const std::string& err_msg) {
    this->line = -1;
    this->position = position;
    this->err_msg = err_msg;
}

bool JsonErrorMatcher::match(const k4json::JsonLoadErr& err) const {
    std::string expected = std::format("Load Error: {}\nline: {} position: {}",
                                       err_msg, line, position);
    std::string gotten = err.what();
    return gotten.find(expected) == 0;
}

bool JsonErrorMatcher::match(const k4json::ExprSyntaxErr& err) const {
    std::string expected = std::format(
        "Json Expression Syntax Error: {}\nposition: {}", err_msg, position);
    std::string gotten = err.what();
    return gotten.find(expected) == 0;
}

bool JsonErrorMatcher::match(const k4json::ExprValueErr& err) const {
    std::string expected = std::format(
        "Json Expression Value Error: {}\nposition: {}", err_msg, position);
    std::string gotten = err.what();
    return gotten.find(expected) == 0;
}

bool JsonErrorMatcher::match(const k4json::JsonTypeErr& err) const {
    return std::string(err.what()).find(err_msg) == 0;
}

std::string JsonErrorMatcher::describe() const {
    std::ostringstream res;
    res << "Equals: \"" << err_msg << '"';
    if (line != -1) {
        res << " line: " << line;
    }
    if (position != -1) {
        res << " position: " << position;
    }
    return res.str();
}

std::string JsonErrorMatcher::toString() const {
    return describe();
}

JsonErrorMatcher EqualsJError(int line, int position,
                              const std::string& err_msg) {
    return JsonErrorMatcher(line, position, err_msg);
}

JsonErrorMatcher EqualsJError(int position, const std::string& err_msg) {
    return JsonErrorMatcher(position, err_msg);
}

JsonErrorMatcher EqualsJError(const std::string& err_msg) {
    return JsonErrorMatcher(err_msg);
}
