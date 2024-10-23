#pragma once

#include <string>

bool is_whitespace(char c);
std::string escape_string(const std::string& str);
std::string pretty_error_pointer(int padding);
