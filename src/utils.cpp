#include "utils.hpp"
#include <string>

bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

std::string escape_string(const std::string& str) {
    std::string res = "";
    for (char c : str) {
        switch (c) {
        case '"':
            res += "\\\"";
            break;
        case '\\':
            res += "\\\\";
            break;
        case '/':
            res += "\\/";
            break;
        case '\b':
            res += "\\b";
            break;
        case '\f':
            res += "\\f";
            break;
        case '\n':
            res += "\\n";
            break;
        case '\r':
            res += "\\r";
            break;
        case '\t':
            res += "\\t";
            break;
        // we don't need to make UTF16 surrogate pairs since
        // we encoded them to UTF8
        default:
            res += c;
        }
    }
    return res;
}

std::string pretty_error_pointer(int padding) {
    std::string res = "";
    for (int i = 0; i < padding; ++i) {
        res += ' ';
    }
    res += '^';
    for (int i = 0; i < 10; ++i) {
        res += '~';
    }
    res += '\n';
    return res;
}
