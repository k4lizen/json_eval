#include "json.hpp"
#include <cassert>

Json::Json() {
    tag = JsonType::NULLVAL;
    val = false; // most lightweight
}

Json::Json(const bool v) {
    tag = JsonType::BOOL;
    val = v;
}

Json::Json(const double num) {
    tag = JsonType::NUMBER;
    val = num;
}

Json::Json(const std::string& str) {
    tag = JsonType::STRING;
    val = str;
}

Json::Json(const JsonType& tag) {
    this->tag = tag;
    switch (tag) {
    case JsonType::OBJECT:
        val = std::map<std::string, Json>();
        break;
    case JsonType::ARRAY:
        val = std::vector<Json>();
        break;
    case JsonType::STRING:
        val = std::string();
        break;
    case JsonType::NUMBER:
        val = 0.0;
        break;
    case JsonType::BOOL:
        val = false;
        break;
    case JsonType::NULLVAL:
        val = false; // most lightweight
        break;
    case JsonType::INVALID:
        assert(0);
    }
}

void Json::array_add(const Json& elem) {
    assert(tag == JsonType::ARRAY);
    std::get<JsonArray>(val).push_back(elem);
}

void Json::obj_add(const std::string& key, const Json& child) {
    assert(tag == JsonType::OBJECT);
    std::get<JsonMap>(val)[key] = child;
}

void Json::obj_add(const KeyedJson& key_val) {
    assert(tag == JsonType::OBJECT);
    std::get<JsonMap>(val)[key_val.first] = key_val.second;
}
