#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

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

class Json;
typedef std::pair<std::string, Json> KeyedJson;
typedef std::map<std::string, Json> JsonMap;
typedef std::vector<Json> JsonArray;

class Json {
  public:
    Json(const bool v);           // bool literal
    Json(const double num);       // number literal
    Json(const std::string& str); // string literal
    Json(const Literal& lit);     // any literal
    Json(const StructureType& tag);
    Json() {}

    void array_add(const Json& child);
    void obj_add(const std::string& key, const Json& child);
    void obj_add(const KeyedJson& key_val);

  private:
    // TODO: remove, can use holds_alternative
    StructureType tag; // meh, double-tagged union
    std::variant<
        // the map is good for wide jsons.
        // for deep jsons, vector would be better.
        // could be optimized with trie
        JsonMap, JsonArray, Literal>
        val;
};

