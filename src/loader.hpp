#pragma once

#include "json.hpp"

#include <string>
#include <stdexcept>

class JsonLoadErr : public std::runtime_error {
  public:
    JsonLoadErr(const std::string& msg) : std::runtime_error(msg) {}
};

class JsonLoader {
  public:
    JsonLoader(const std::string& file_name);

  private:
    std::string buffer;
    unsigned int line = 1;    // line currently being parsed
    unsigned int current = 0; // index of character being parsed
    Json root;           // the deserialized json

    [[noreturn]] void load_err(const std::string& msg);
    std::string error_line();
    Json load();

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
