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
	Literal(std::string str);
	Literal(bool v);
	Literal(double num);
	Literal(LiteralType lt);  // should be used only for NULLVAL
};

enum class StructureType {
	INVALID = 0,
	OBJECT,
	ARRAY,
	LITERAL
};

class Structure;
typedef std::pair<std::string, Structure> KeyedStructure;
typedef std::map<std::string, Structure> StructureMap;
typedef std::vector<Structure> StructureArray;

class Structure {
public:
	// TODO: remove, can use holds_alternative
	StructureType tag; // meh, double-tagged union
	std::variant<
			// the map is good for wide jsons
			// for deep jsons, vector would be better
			// could be optimized with trie
		    StructureMap,
			StructureArray,
			Literal
		> val;

	Structure(std::string str); // string literal
	Structure(bool v);          // bool literal 
	Structure(double num);      // number literal
	Structure(Literal lit);     // any literal
	Structure(StructureType tag);
	Structure() {}
	
	void array_add(Structure child);
	void obj_add(std::string key, Structure child);
	void obj_add(KeyedStructure key_val);
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
	void line_err();
	Structure load();

	char peek();
	char next();
	bool match(char c);
	void skip();
	bool reached_end();
	Structure load_object();
	Structure load_array();
	std::pair<std::string, Structure> load_pair();
	Structure load_value();
	std::string load_string();
	bool match_true();
	bool match_false();
	bool match_null();
	bool match_number(double& number);
};
