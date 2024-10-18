#include "loader.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "usage: ./json_eval <json file> <query>" << std::endl;
        return 1;
    }

    try {
        // Load and parse json from file
        JsonLoader json((std::string(argv[1])));
    } catch (const JsonLoadErr& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "File OK!\n";

    return 0;
}
