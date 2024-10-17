#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>

#include "json.h"

bool is_whitespace(char c){
    return c == ' ' || c == '\n' || c == '\t' || c == '\r'; 
}

void Json::load_err(std::string msg){
    std::cerr << "Load Error: " << msg << std::endl;
    std::cerr << "line: " << line << std::endl;
    exit(1);
}

Json::Json(std::string file_name){
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
bool Json::match(char c){
    if(peek() == c){
        next();
        return true;
    }
    return false;
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

void Structure::add(std::string key, Structure child){
    assert(tag == StructureType::OBJECT);
    std::get<std::map<std::string, Structure>>(val)[key] = child;
}

void Structure::add(KeyedStructure key_val){
    assert(tag == StructureType::OBJECT);
    std::get<std::map<std::string, Structure>>(val)[key_val.first] = key_val.second;
}

void Structure::set(std::string str){
    assert(tag == StructureType::LITERAL);
    std::get<Literal>(val).set_string(str);
}

void Literal::set_string(std::string str){
    tag = LiteralType::STRING;
    val = str;
}

char get_escaped(){
    // string section of https://www.json.org/json-en.html
    // TODO
    return '\0';
}

std::string Json::load_string(){
    assert(match('\"'));

    std::stringstream sstream;

    while(!reached_end()){
        char c = peek();
        
        switch(c){
        case '\"':
            next();
            return sstream.str();
        case '\\':
            sstream << get_escaped();
            break;
        default:
            sstream << c;
        }

        next();
    }

    load_err("unterminated string");    
}


// should error out if there is nothing to load
Structure Json::load_something(){
    Structure val(StructureType::LITERAL); // WTF
    
    switch(peek()){
    case '{':
        return load_object();
    case '[':
        return load_array();
    case '\"':
        val.set(load_string());
        return val;
    default:
        // could be number, string, true, false, null
        break;
    }
}

KeyedStructure Json::load_pair(){
    assert(peek() == '\"');

    std::string key = load_string();
    skip();
    
    if(!match(':')){
        load_err("key string must be followed by a semicolon");    
    }

    skip();
    Structure val = load_something();

    return KeyedStructure(key, val);
}


Structure Json::load_object(){
    assert(match('{'));

    Structure node(StructureType::OBJECT);
    bool need = false;

    while(!reached_end()){
        skip();
        
        switch(peek()){
        case '}':
            if (need){
                load_err("expected one more element. ,} isn't valid");
            }
            return node; // end of current object
        case '\"':
            node.add(load_pair());
            need = false;
            break;
        case ',':
            next();
            need = true;   // ,} is not valid json
            break;
        default:
            load_err("key must be in double-quotes");
            break;
        }
    }

    load_err("reached end of file without closing bracket: }");
}

Structure Json::load_array(){
    assert(match('['));
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


Structure::Structure(StructureType tag){
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
