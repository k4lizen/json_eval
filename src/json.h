#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class JsonLoadErr : public std::runtime_error {
  public:
    JsonLoadErr(const std::string& msg) : std::runtime_error(msg) {}
};

enum class LiteralType {
    INVALID = 0,
    STRING,
    NUMBER,
    BOOL,
    NULLVAL
};

// TODO: should be struct?
class Literal {
  public:
    Literal();
    Literal(const std::string& str);
    Literal(const bool v);
    Literal(const double num);
    Literal(const LiteralType& lt); // should be used only for NULLVAL

  private:
    LiteralType tag; // double-tagging needed for nullval
    std::variant<std::string, double, bool> val;
};

enum class StructureType {
    INVALID = 0,
    OBJECT,
    ARRAY,
    LITERAL
};

class Structure;
typedef std::pair<std::string, Structure> KeyedStructure;
typedef std::map<std::string, Structure> StructureMap;
typedef std::vector<Structure> StructureArray;

class Structure {
  public:
    Structure(const bool v);           // bool literal
    Structure(const double num);       // number literal
    Structure(const std::string& str); // string literal
    Structure(const Literal& lit);     // any literal
    Structure(const StructureType& tag);
    Structure() {}

    void array_add(const Structure& child);
    void obj_add(const std::string& key, const Structure& child);
    void obj_add(const KeyedStructure& key_val);

  private:
    // TODO: remove, can use holds_alternative
    StructureType tag; // meh, double-tagged union
    std::variant<
        // the map is good for wide jsons
        // for deep jsons, vector would be better
        // could be optimized with trie
        StructureMap, StructureArray, Literal>
        val;
};

class Json {
  public:
    Json(const std::string& file_name);

  private:
    std::string buffer;
    unsigned int line = 1;    // line currently being parsed
    unsigned int current = 0; // index of character being parsed
    Structure root;           // the deserialized json

    [[noreturn]] void load_err(const std::string& msg);
    std::string error_line();
    Structure load();

    char peek();
    char next();
    bool match(const char c);
    void assert_match(const char c);
    void skip();
    bool reached_end();
    Structure load_object();
    Structure load_array();
    KeyedStructure load_pair();
    Structure load_value();
    std::string load_string();
    char parse_escaped();
    bool match_true();
    bool match_false();
    bool match_null();
    bool match_number(double& number);
};
