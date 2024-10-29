#include "expressions.hpp"
#include "json.hpp"
#include "loader.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "usage: ./json_eval <json file> <query>" << '\n';
        return 1;
    }

    using namespace k4json;

    Json json;
    try {
        // Load and parse json from file
        json = from_file((std::string(argv[1])));
    } catch (const JsonLoadErr& e) {
        std::cerr << e.what() << '\n';
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    try {
        // Parse the query
        JsonArray result = parse(json, std::string(argv[2]));
        // If the resulting JsonArray is only one element
        // we will extract it
        if (result.size() == 1) {
            std::cout << result[0].to_string() << '\n';
        } else {
            std::cout << Json(result).to_string() << '\n';
        }
    } catch (const JsonTypeErr& e) {
        std::cerr << e.what() << '\n';
        return 3;
    } catch (const ExprSyntaxErr& e) {
        std::cerr << e.what() << '\n';
        return 4;
    } catch (const ExprValueErr& e) {
        std::cerr << e.what() << '\n';
        return 5;
    }

    return 0;
}
