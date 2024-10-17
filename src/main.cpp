#include <iostream>
#include "json.h"

int main(int argc, char* argv[]){
    if(argc != 3){
        std::cout << "usage: ./json_eval <json file> <query>" << std::endl;
        return 1;
    }

    Json json(argv[1]);    
}
