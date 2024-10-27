#pragma once

#include <string>

namespace k4json {

bool is_whitespace(char c);
std::string escape_string(const std::string& str);
std::string pretty_error_pointer(int padding);

} // namespace k4json
