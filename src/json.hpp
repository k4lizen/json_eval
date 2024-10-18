#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

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
    Json();                       // null literal
    Json(const bool v);           // true / false literal
    Json(const double num);       // number literal
    Json(const std::string& str); // string literal
    Json(const JsonType& tag);    // Anything (used for object and array)

    void array_add(const Json& child);
    void obj_add(const KeyedJson& key_val);

private:
    JsonType tag; // meh, double-tagged union
    std::variant<JsonMap, JsonArray, std::string, double, bool> val;
};
