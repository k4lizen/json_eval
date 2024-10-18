#pragma once

#include "json.hpp"

#include <stdexcept>
#include <string>

class JsonLoadErr : public std::runtime_error {
public:
    JsonLoadErr(const std::string& msg) : std::runtime_error(msg) {}
};

// Used for deserializing JSON
// returns object of type Json
class JsonLoader {
public:
    static Json from_string(const std::string& str);
    static Json from_file(const std::string& file_name);

private:
    JsonLoader(const std::string& data);

    std::string buffer;
    unsigned int line;    // line currently being parsed
    unsigned int current; // index of character being parsed

    [[noreturn]] void load_err(const std::string& msg);
    std::string error_line();
    Json load(bool strict = true);

    char peek();
    char next();
    bool match(const char c);
    void assert_match(const char c);
    void skip();
    bool reached_end();
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
    bool match_number(double& number);
};
