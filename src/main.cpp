#include "expressions.hpp"
#include "json.hpp"
#include "loader.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "usage: ./json_eval <json file> <query>" << '\n';
        return 1;
    }

    Json json;
    try {
        // Load and parse json from file
        json = Json::from_file((std::string(argv[1])));
    } catch (const JsonLoadErr& e) {
        std::cerr << e.what() << '\n';
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        return 3;
    }

    try {
        Json result = json.evaluate_expr(std::string(argv[2]));
        // If the resulting JsonArray is only one element
        // we will extract it
        if (result.get_type() == JsonType::ARRAY && result.size() == 1) {
            result = result[0];
        }
        std::cout << result.to_string() << '\n';
    } catch (const JsonTypeErr& e) {
        std::cerr << e.what() << '\n';
        return 2;
    } catch (const JsonExprErr& e) {
        std::cerr << e.what() << '\n';
        return 7;
    }

    return 0;
}
