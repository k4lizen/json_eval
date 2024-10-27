#pragma once

#include "json.hpp"
#include "parser.hpp"

#include <stdexcept>
#include <string>

namespace k4json {

class JsonLoadErr : public std::runtime_error {
public:
    explicit JsonLoadErr(const std::string& msg) : std::runtime_error(msg) {}
};

// Used for deserializing JSON
// returns object of type Json
class JsonLoader : private Parser {
public:
    static Json from_string(const std::string& str);
    static Json from_file(const std::string& file_name);

private:
    explicit JsonLoader(const std::string& data);

    [[noreturn]] void syntax_err(const std::string& msg) override;
    std::string error_line();
    Json load(bool strict = true);

    Json load_object();
    Json load_array();
    KeyedJson load_pair();
    Json load_value();
    std::string load_string();
    std::string parse_escaped();
    std::string parse_unicode();
    unsigned int parse_codepoint();
    unsigned int unhexbyte();

    bool match_true();
    bool match_false();
    bool match_null();
};

} // namespace k4json
