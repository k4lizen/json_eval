#define CATCH_CONFIG_MAIN

#include "loader.hpp"
#include "catch_amalgamated.hpp"


TEST_CASE ("no value") {
    std::string filename("tests/data/no_value.json");
    std::string expected(
        "Load Error: unexpected symbol for value\n"
        "line: 21 position: 375\n"
        "...\n"
        "20:            \"awa\":\n"
        "21:        },\n"
        "           ^~~~~~~~~~~\n");

    REQUIRE_THROWS_AS(
        [filename] {
            JsonLoader json(filename);
        }(),
        JsonLoadErr
    );

    REQUIRE_THROWS_WITH(
        [filename] {
            JsonLoader json(filename);
        }(),
        expected
    );
}
