#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class JsonTypeErr : public std::runtime_error {
public:
    JsonTypeErr(const std::string& msg) : std::runtime_error(msg) {}
};


enum class JsonType {
    INVALID = 0,
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    BOOL,
    NULLVAL
};

class Json;
typedef std::pair<std::string, Json> KeyedJson;
// The map is good for wide jsons. For deep jsons, vector would be better.
// Could be optimized with trie
typedef std::map<std::string, Json> JsonMap;
typedef std::vector<Json> JsonArray;

// Class used to represent a JSON object in memory.
// To get an object of this type you probably want to use
// JsonLoader::from_string() or JsonLoader::from_file().
class Json {
public:
    Json();                        // null literal
    Json(const bool v);            // true / false literal
    Json(const double num);        // number literal
    Json(const std::string& str);  // string literal
    Json(const JsonMap& jmap);     // object
    Json(const JsonArray& jarray); // array

    std::string evaluate_expr(const std::string& expr);

    // modifiers
    void array_add(const Json& child);
    void obj_add(const KeyedJson& key_val);

    // accessors
    JsonType get_type();
    bool is_null();
    bool get_bool();
    double get_number();
    std::string get_string();

    Json operator[](const int idx);
    Json operator[](const std::string& key);
    bool obj_contains(const std::string& key);
    int size();
    
private:
    bool _is_null;
    std::variant<JsonMap, JsonArray, std::string, double, bool> val;
};
