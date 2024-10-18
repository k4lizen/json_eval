#define CATCH_CONFIG_MAIN

#include "loader.hpp"
#include "catch_amalgamated.hpp"


TEST_CASE ("no value") {
    std::string data = R"({
    "mama mia": {
        "4": 4,
        "this is": "pretty cool",
        "key": {
            "a": true,
            "b": false,
            "c": null
        },
        "arr": [
            "a",
            "b",
            [],
            null,
            true,
            3.13
        ],
        "num": -10120.23029E+39,
        "nya": {
            "awa":
        },
        "filler": []
    },
})";

    std::string expected = R"(Load Error: unexpected symbol for value
line: 21 position: 375
...
20:            "awa":
21:        },
           ^~~~~~~~~~~
)";

    REQUIRE_THROWS_AS(
        [data] {
            JsonLoader::from_string(data);
        }(),
        JsonLoadErr
    );

    REQUIRE_THROWS_WITH(
        [data] {
            JsonLoader::from_string(data);
        }(),
        expected
    );
}
