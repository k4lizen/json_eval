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

    Json evaluate_expr(const std::string& expr) const;

    // modifiers
    void array_add(const Json& child);
    void obj_add(const KeyedJson& key_val);

    // accessors
    JsonType get_type() const;
    bool is_null() const;
    bool get_bool() const;
    double get_number() const;
    std::string get_string() const;

    Json operator[](const int idx) const;
    Json operator[](const std::string& key) const;
    bool obj_contains(const std::string& key) const;
    std::vector<std::string> get_obj_keys() const; 
    int size() const;

    // serialize the json object to a string
    std::string to_string() const;

private:
    std::string to_string(int indent) const;
    
    bool _is_null;
    std::variant<JsonMap, JsonArray, std::string, double, bool> val;
};
