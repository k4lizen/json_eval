#include "json.hpp"
#include "expressions.hpp"
#include "loader.hpp"
#include "utils.hpp"
#include <cassert>
#include <variant>

Json::Json() {
    _is_null = true;
    val = false; // set to null just in case
}

Json::Json(const bool v) {
    _is_null = false;
    val = v;
}

Json::Json(const double num) {
    _is_null = false;
    val = num;
}

Json::Json(const std::string& str) {
    _is_null = false;
    val = str;
}

Json::Json(const JsonMap& jmap) {
    _is_null = false;
    val = jmap;
}

Json::Json(const JsonArray& jarray) {
    _is_null = false;
    val = jarray;
}

Json Json::evaluate_expr(const std::string& expr) {
    return JsonExpressionParser::parse(*this, expr);
}

// valid only for JsonType::ARRAY
void Json::array_add(const Json& elem) {
    if (std::holds_alternative<JsonArray>(val)) {
        std::get<JsonArray>(val).push_back(elem);
    } else {
        throw JsonTypeErr("cannot array_add(), instance isnt JsonType::ARRAY");
    }
}

// valid only for JsonType::OBJECT
void Json::obj_add(const KeyedJson& key_val) {
    if (std::holds_alternative<JsonMap>(val)) {
        std::get<JsonMap>(val)[key_val.first] = key_val.second;
    } else {
        throw JsonTypeErr("cannot obj_add(), instance isnt JsonType::OBJECT");
    }
}

JsonType Json::get_type() {
    if (_is_null) {
        return JsonType::NULLVAL;
    } else if (std::holds_alternative<bool>(val)) {
        return JsonType::BOOL;
    } else if (std::holds_alternative<double>(val)) {
        return JsonType::NUMBER;
    } else if (std::holds_alternative<std::string>(val)) {
        return JsonType::STRING;
    } else if (std::holds_alternative<JsonArray>(val)) {
        return JsonType::ARRAY;
    } else {
        return JsonType::OBJECT;
    }
}

bool Json::is_null() {
    return _is_null;
}

// valid only for JsonType::BOOL
bool Json::get_bool() {
    if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val);
    }
    throw JsonTypeErr("get_bool() called on Json which isnt JsonType::BOOL");
}

// valid only for JsonType::NUMBER
double Json::get_number() {
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);
    }
    throw JsonTypeErr("get_number() called on Json which isnt JsonType::NUMBER");
}

// valid only for JsonType::STRING
std::string Json::get_string() {
    if (std::holds_alternative<std::string>(val)) {
        return std::get<std::string>(val);
    }
    throw JsonTypeErr("get_string() called on Json which isnt JsonType::STRING");
}

// valid only for JsonType::ARRAY
Json Json::operator[](const int idx) {
    if (std::holds_alternative<JsonArray>(val)) {
        return std::get<JsonArray>(val).at(idx);
    } else {
        throw JsonTypeErr("operator[int] invalid, instance isnt JsonType::ARRAY");
    }
}

// valid only for JsonType::OBJECT
Json Json::operator[](const std::string& key) {
    if (std::holds_alternative<JsonMap>(val)) {
        auto obj = std::get<JsonMap>(val);
        if (obj.contains(key)) {
            return obj[key];
        } else {
            throw JsonTypeErr("object doesn't contain provided key");
        }
    } else {
        throw JsonTypeErr("operator[std::string] invalid, instance isnt JsonType::OBJECT");
    }
}

// valid only for JsonType::OBJECT
bool Json::obj_contains(const std::string& key) {
    if (std::holds_alternative<JsonMap>(val)) {
        return std::get<JsonMap>(val).contains(key);
    } else {
        throw JsonTypeErr("obj_contains called on Json which isnt JsonType::OBJECT");
    }
}

// Returns the number of Jsons inside this one, shallowly
int Json::size(){
    if (_is_null) {
        return 0;
    } else if (std::holds_alternative<bool>(val)) {
        return 0;
    } else if (std::holds_alternative<double>(val)) {
        return 0;
    } else if (std::holds_alternative<std::string>(val)) {
        return 0;
    } else if (std::holds_alternative<JsonArray>(val)) {
        return std::get<JsonArray>(val).size();
    } else {
        return std::get<JsonMap>(val).size();
    }
}

Json Json::from_string(const std::string& str) {
    return JsonLoader::from_string(str);
}

Json Json::from_file(const std::string& file_name) {
    return JsonLoader::from_file(file_name);
}

std::vector<std::string> Json::get_obj_keys(){
    if (!std::holds_alternative<JsonMap>(val)) {
        throw JsonTypeErr("get_obj_keys() called on Json which isnt JsonType::OBJECT");
    }

    JsonMap jmap = std::get<JsonMap>(val);
    std::vector<std::string> keys;
    keys.reserve(jmap.size());
    for(auto it = jmap.begin(); it != jmap.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

std::string Json::to_string() {
    return to_string(1);
}

std::string Json::to_string(int indent) {
    // using 4-space indentation
    std::string s_indent_less((indent - 1) * 4, ' ');
    std::string s_indent = s_indent_less + "    ";
    std::string res;

    switch (get_type()) {
    case JsonType::OBJECT: {
        if (size() == 0) {
            return "{ }";
        }
        res = "{\n";
        std::vector<std::string> keys = get_obj_keys();
        int n = keys.size();
        for(int i = 0; i < n - 1; ++i) {
            res += s_indent + '"' + keys[i] + "\": " + (*this)[keys[i]].to_string(indent+1) + ",\n";
        }
        res += s_indent + '"' + keys[n - 1] + "\": " + (*this)[keys[n - 1]].to_string(indent+1) + '\n';
        res += s_indent_less + "}";
        return res;
    }
    case JsonType::ARRAY: {
        if (size() == 0) {
            return "[ ]";
        }
        res = "[\n";
        for (int i = 0; i < size() - 1; ++i){
            res += s_indent + (*this)[i].to_string(indent+1) + ",\n";
        }
        res += s_indent + (*this)[size() - 1].to_string(indent+1) + '\n';
        res += s_indent_less + "]";
        return res;
    }
    case JsonType::STRING:
        return '"' + escape_string(get_string()) + '"';
    case JsonType::NUMBER:
        return std::to_string(get_number());
    case JsonType::BOOL:
        return get_bool() ? "true" : "false";
    case JsonType::NULLVAL:
        return "null";
    case JsonType::INVALID:
    default:
        throw JsonTypeErr("Serialization failed, impossible json type.");
    }
}
