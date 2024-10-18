#include "loader.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "usage: ./json_eval <json file> <query>" << std::endl;
        return 1;
    }

    try {
        // Load and parse json from file
        Json json = JsonLoader::from_file((std::string(argv[1])));
    } catch (const JsonLoadErr& e) {
        std::cerr << e.what() << '\n';
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    std::cout << "File OK!\n";

    return 0;
}
