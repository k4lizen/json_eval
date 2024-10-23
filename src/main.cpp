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
        std::cout << "File OK!\n";

    } catch (const JsonLoadErr& e) {
        std::cerr << e.what() << '\n';
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        return 3;
    }

    try {
        Json result = json.evaluate_expr(std::string(argv[2]));
        std::cout << result.to_string() << '\n';
    } catch (const JsonTypeErr& e) {
        std::cerr << e.what() << '\n';
        return 2;
    }
    return 0;
}
