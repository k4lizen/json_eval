#include "err_matcher.hpp"

#include <format>

JsonErrorMatcher::JsonErrorMatcher(int line, int position,
                                   const std::string& err_msg) {
    this->line = line;
    this->position = position;
    this->err_msg = err_msg;
}

bool JsonErrorMatcher::match(const JsonLoadErr& err) const {
    std::string expected = std::format("Load Error: {}\nline: {} position: {}", err_msg, line, position);
    std::string gotten = err.what();
    return gotten.find(expected) == 0;
}

std::string JsonErrorMatcher::describe() const {
    std::ostringstream res;
    res << "Equals: \"" << err_msg << '"' << " line: " << line
        << " position: " << position;
    return res.str();
}

std::string JsonErrorMatcher::toString() const {
    return describe();
}

JsonErrorMatcher EqualsJError(int line, int position,
                              const std::string& err_msg) {
    return JsonErrorMatcher(line, position, err_msg);
}
