#include "json.hpp"
#include <cassert>

Literal::Literal() : tag(LiteralType::INVALID) {}

Literal::Literal(const LiteralType& lt) {
    tag = lt;
    val = false; // setting the memory to 0 just in case
}

Literal::Literal(const std::string& str) {
    tag = LiteralType::STRING;
    val = str;
}

Literal::Literal(const bool v) {
    tag = LiteralType::BOOL;
    val = v;
}

Literal::Literal(const double num) {
    tag = LiteralType::NUMBER;
    val = num;
}

Json::Json(const double num) {
    tag = StructureType::LITERAL;
    val = Literal(num);
}

Json::Json(const Literal& lit) {
    tag = StructureType::LITERAL;
    val = lit;
}

Json::Json(const std::string& str) {
    tag = StructureType::LITERAL;
    val = Literal(str);
}

Json::Json(const StructureType& tag) {
    this->tag = tag;
    switch (tag) {
    case StructureType::OBJECT:
        val = std::map<std::string, Json>();
        break;
    case StructureType::ARRAY:
        val = std::vector<Json>();
        break;
    case StructureType::LITERAL:
        val = Literal();
        break;
    case StructureType::INVALID:
        assert(0);
    }
}

Json::Json(const bool v) {
    tag = StructureType::LITERAL;
    val = Literal(v);
}

void Json::array_add(const Json& elem) {
    assert(tag == StructureType::ARRAY);
    std::get<JsonArray>(val).push_back(elem);
}

void Json::obj_add(const std::string& key, const Json& child) {
    assert(tag == StructureType::OBJECT);
    std::get<JsonMap>(val)[key] = child;
}

void Json::obj_add(const KeyedJson& key_val) {
    assert(tag == StructureType::OBJECT);
    std::get<JsonMap>(val)[key_val.first] = key_val.second;
}
