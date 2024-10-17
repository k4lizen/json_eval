#include <cassert>
#include <charconv>
#include <iostream>
#include <sstream>
#include <fstream>

#include "json.h"

bool is_whitespace(char c){
    return c == ' ' || c == '\n' || c == '\t' || c == '\r'; 
}

// return a description of the location of the error
std::string Json::error_line(){
    std::string desc = "line: " + std::to_string(line) + '\n';
    
    int ln_start, ln_end;
    ln_start = ln_end = current;
    while(ln_start >= 0 && buffer[ln_start] != '\n'){
        ln_start--;
    }
    while(ln_end < static_cast<int>(buffer.size()) && buffer[ln_end] != '\n'){
        ln_end++;
    }

    // return the line which caused the issue
    desc += buffer.substr(ln_start + 1, (ln_end - ln_start));

    // point to the exact problematic symbol
    for(int i = ln_start + 1; i < static_cast<int>(current); ++i){
        desc += ' ';
    }
    desc += '^';
    for(int i = 0; i < 10; ++i){
        desc += '~';
    }
    desc += '\n';
    
    return desc;
}

[[noreturn]] void Json::load_err(const std::string& msg){
    std::string err_msg =  "Load Error: " + msg + '\n';
    err_msg += error_line();

    throw JsonLoadErr(err_msg);
}

Json::Json(const std::string& file_name){
    std::ifstream infile(file_name);

    if(!infile.good()){
        infile.close();
        load_err("failed opening file \"" + file_name + "\". Does it exist?");
    }
    
    std::stringstream sstr;
    infile >> sstr.rdbuf();
    buffer = sstr.str();
    infile.close();

    if(buffer.empty()){
        load_err("file \"" + file_name + "\" is empty.");
    }

    std::cout << "File contents: " << std::endl << buffer;   

    // Main parsing logic
    root = load();
}

// Advance only if the next character matches param
bool Json::match(const char c){
    if(peek() == c){
        next();
        return true;
    }
    return false;
}

void Json::assert_match(const char c){
    assert(peek() == c);
    next();
}

// Return current character
char Json::peek(){
    if(reached_end()){
        return '\0';
    }
    
    return buffer[current];
}

// Advance and return next character
char Json::next(){
    if(reached_end() || current + 1 >= buffer.size()){
        return '\0';
    }
    
    return buffer[++current];
}

bool Json::reached_end(){
    return current >= buffer.size();
}

// Skip all whitespace
void Json::skip(){
    // bounds checking is happening in peek() and next()
    char c = peek();
    while(is_whitespace(c)){
        if(c == '\n'){
            line++;
        }
        c = next();
    }
}

void Structure::array_add(const Structure& elem){
    assert(tag == StructureType::ARRAY);
    std::get<StructureArray>(val).push_back(elem);
}

void Structure::obj_add(const std::string& key, const Structure& child){
    assert(tag == StructureType::OBJECT);
    std::get<StructureMap>(val)[key] = child;
}

void Structure::obj_add(const KeyedStructure& key_val){
    assert(tag == StructureType::OBJECT);
    std::get<StructureMap>(val)[key_val.first] = key_val.second;
}

char Json::parse_escaped(){
    // string section of https://www.json.org/json-en.html
    assert_match('\\');

    switch(peek()){
    case '"':
        return '"';
    case '\\':
        return '\\';
    case '/':
        return '/';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'u':
        // unicode
        break;
    default:
        load_err("invalid escape sequence");
    }
}

std::string Json::load_string(){
    assert_match('"');

    std::stringstream sstream;

    while(!reached_end()){
        char c = peek();
        
        switch(c){
        case '"':
            next();
            return sstream.str();
        case '\\':
            sstream << parse_escaped();
            break;
        default:
            sstream << c;
        }

        next();
    }

    load_err("unterminated string");    
}

bool is_end_control(const char c){
    return c == ',' || c == '}' || c == ']';
}

bool Json::match_false(){
    if(current + 5 < buffer.size() &&
        buffer[current]     == 'f' &&
        buffer[current + 1] == 'a' &&
        buffer[current + 2] == 'l' &&
        buffer[current + 3] == 's' &&
        buffer[current + 4] == 'e' &&
        // need to make sure it isn't something like falseatswa, using whitelist  
        (is_end_control(buffer[current + 5]) || is_whitespace(buffer[current + 5]))){
        
        current += 5;
        return true;
    }    

    return false;
}

bool Json::match_true(){
    if(current + 4 < buffer.size() &&
        buffer[current]     == 't' &&
        buffer[current + 1] == 'r' &&
        buffer[current + 2] == 'u' &&
        buffer[current + 3] == 'e' &&
        // need to make sure it isn't something like trueatswa, using whitelist  
        (is_end_control(buffer[current + 4]) || is_whitespace(buffer[current + 4]))){
        
        current += 4;
        return true;
    }    

    return false;
}

bool Json::match_null(){
    if(current + 4 < buffer.size() &&
        buffer[current]     == 'n' &&
        buffer[current + 1] == 'u' &&
        buffer[current + 2] == 'l' &&
        buffer[current + 3] == 'l' &&
        // need to make sure it isn't something like nullatswa, using whitelist  
        (is_end_control(buffer[current + 4]) || is_whitespace(buffer[current + 4]))){
        
        current += 4;
        return true;
    }    

    return false;
}

// If false is returned, number is undefined
bool Json::match_number(double& number){
    // We can't use strtod and friends because they don't
    // conform to the json definition of a number, but
    // from_chars does. Also, see Notes section in
    // https://en.cppreference.com/w/cpp/utility/from_chars

    const char* cbuff = buffer.c_str();
    // We give it the whole rest of the buffer, it will only care about the initial valid part
    auto [ptr, ec] = std::from_chars(cbuff + current, cbuff + buffer.size() - 1, number);

    if(ec == std::errc::invalid_argument){
        // no number at that location
        return false;
    }

    if(ec == std::errc::result_out_of_range){
        load_err("number out of range");
    }

    // valid number!
    current = ptr - cbuff;
    return true;
}

Structure Json::load_value(){
    skip();    
    
    switch(peek()){
    case '{':
        return load_object();
    case '[':
        return load_array();
    case '"':
        return Structure(load_string());
    default:
        if(match_true()){
            return Structure(true);
        }
        if(match_false()){
            return Structure(false);
        }
        if(match_null()){
            return Structure(Literal(LiteralType::NULLVAL));
        }
        double number;
        if(match_number(number)){
            return Structure(number);
        }

        load_err("unexpected symbol for value");
    }
}

KeyedStructure Json::load_pair(){
    assert(peek() == '"');

    std::string key = load_string();
    skip();
    
    if(!match(':')){
        load_err("key string must be followed by a semicolon");    
    }

    Structure val = load_value();

    return KeyedStructure(key, val);
}


Structure Json::load_object(){
    assert_match('{');

    Structure node(StructureType::OBJECT);

    // empty object
    skip();
    if(match('}')){
        return node;
    }

    // whether we are expecting another item in the object
    bool pending = true;

    while(!reached_end()){
        skip();

        if(pending){
            if(peek() == '\"'){
                node.obj_add(load_pair());
                pending = false;
                continue;
            }else{
                load_err("unexpected symbol, wanted key-value pair");
            }
        }

        // not currently pending
        switch(peek()){
        case ',':
            pending = true;
            next();
            break;
        case '}':
            next();
            return node;
        default:
            load_err("unexpected symbol, wanted , or }");
        }
    }

    load_err("reached EOF without closing curly brace");
}

Structure Json::load_array(){
    assert_match('[');

    Structure node(StructureType::ARRAY);

    // empty array
    skip();
    if(match(']')){
        return node;
    }
    
    // whether we are expecting another item in the array
    bool pending = true;
   
    while(!reached_end()){
        skip();

        if(pending){
            node.array_add(load_value());
            pending = false;
            continue;
        }

        // not currently pending
        switch(peek()){
        case ']':
            next();
            return node;
        case ',':
            pending = true;
            next();
            break;
        default:
            load_err("unexpected symbol, wanted , or ]");
        }
    }
    
    load_err("reached EOF without closing square brace");
}


Structure Json::load(){
    // The most outer structure is handled seperately 
    // since it must be an object or array
    skip();
    
    switch(peek()){
    case '{':
        return load_object();
    case '[':
        return load_array();
    default:
        load_err("json must be object or array");
    }
}

Literal::Literal() : tag(LiteralType::INVALID) {}

Literal::Literal(const LiteralType& lt){
    tag = lt;
    val = false; // setting the memory to 0 just in case
}

Literal::Literal(const std::string& str){
    tag = LiteralType::STRING;
    val = str;
}

Literal::Literal(const bool v){
    tag = LiteralType::BOOL;
    val = v;
}

Literal::Literal(const double num){
    tag = LiteralType::NUMBER;
    val = num;
}

Structure::Structure(const double num){
    tag = StructureType::LITERAL;
    val = Literal(num);
}

Structure::Structure(const Literal& lit){
    tag = StructureType::LITERAL;
    val = lit;
}

Structure::Structure(const std::string& str){
    tag = StructureType::LITERAL;
    val = Literal(str);
}

Structure::Structure(const StructureType& tag){
	this->tag = tag;
	switch (tag) {
    	case StructureType::OBJECT:
	    	val = std::map<std::string, Structure>();
	    	break;
	    case StructureType::ARRAY:
	    	val = std::vector<Structure>();
	    	break;
	    case StructureType::LITERAL:
	    	val = Literal();
	    	break;
	    case StructureType::INVALID:
	    	exit(2); // should never be reached
    }
}

Structure::Structure(const bool v){
    tag = StructureType::LITERAL;
    val = Literal(v);
}
