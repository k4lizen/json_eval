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
	LiteralType tag; // double-tagging needed for nullval
	std::variant<
			std::string,
			double,
			bool 
		> val;

	Literal();
	void set_string(std::string str); 
};

enum class StructureType {
	INVALID = 0,
	OBJECT,
	ARRAY,
	LITERAL
};

class Structure;
typedef std::pair<std::string, Structure> KeyedStructure;

class Structure {
public:
	// TODO: remove, can use holds_alternative
	StructureType tag; // meh, double-tagged union
	std::variant<
		    std::map<std::string, Structure>,
			std::vector<Structure>,
			Literal
		> val;
	
	Structure(StructureType tag);
	Structure() {}
	
	// for array (vector)
	void add(Structure child);
	// for object (map)
	void add(std::string key, Structure child);
	void add(KeyedStructure key_val);
	// for literals
	void set(std::string str);
};

class Json{
public:
	Json(std::string file_name);
private:
	std::string buffer;
	unsigned int line = 0;      // line currently being parsed
	unsigned int current = 0;  // index of character being parsed
	unsigned int start = 0;     // start of token being parsed 
	Structure root;         // the deserialized json

	void load_err(std::string);
	Structure load();

	char peek();
	char next();
	bool match(char c);
	void skip();
	bool reached_end();
	Structure load_object();
	Structure load_array();
	std::pair<std::string, Structure> load_pair();
	Structure load_something();
	std::string load_string();
};
