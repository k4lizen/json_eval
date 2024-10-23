#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class JsonTypeErr : public std::runtime_error {
public:
    explicit JsonTypeErr(const std::string& msg) : std::runtime_error(msg) {}
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
class Json {
public:
    Json();                                 // null literal
    explicit Json(const bool v);            // true / false literal
    explicit Json(const double num);        // number literal
    explicit Json(const std::string& str);  // string literal
    explicit Json(const JsonMap& jmap);     // object
    explicit Json(const JsonArray& jarray); // array

    static Json from_string(const std::string& str);
    static Json from_file(const std::string& file_name);

    Json evaluate_expr(const std::string& expr);

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
    std::vector<std::string> get_obj_keys(); 
    int size();

    // serialize the json object to a string
    std::string to_string();

private:
    std::string to_string(int indent);
    
    bool _is_null;
    std::variant<JsonMap, JsonArray, std::string, double, bool> val;
};
