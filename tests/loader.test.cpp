#define CATCH_CONFIG_MAIN

#include "loader.hpp"
#include "catch_amalgamated.hpp"

std::string data_loc = "tests/data/";

TEST_CASE("all ok (string)", "[loader]") {
    std::string data = R"({
    "mama mia": {
        "4": 4,
        "this is": "pretty cool",
        "nya": {
            "awa": "peko"
        },
        "key": {
            "a": true,
            "b": false,
            "c": null
        },
        "arr": [
            "a",
            "b\n\fcc\rc\b\n\na\"quotes\"",
            [],
            null,
            true,
            3.13,
            { "a": {} },
            [ "a", {"b": ["c"] }]
        ],
        "num": -10120.23029E+39
    },
    "is cooking": "something\u000A\u0041bb\uAAAA\uD801\uDD01nya"
})";

    REQUIRE_NOTHROW([data] {
        JsonLoader::from_string(data);
    }());
}

TEST_CASE("all ok (file)", "[loader][file]") {
    REQUIRE_NOTHROW([] {
        JsonLoader::from_file(data_loc + "ok.json");
    }());
}

TEST_CASE("file doesnt exit", "[loader][file]") {
    std::string fname = "THISFILEDOESNTEXISTA0RSiTNR";
    std::string expected = "Failed opening file " + fname + ". Does it exit?";

    REQUIRE_THROWS_MATCHES(
        [fname] {
            JsonLoader::from_file(fname);
        }(),
        std::runtime_error, Catch::Matchers::Message(expected));
}

TEST_CASE("file empty", "[loader][file]") {
    std::string fname = "empty.json";
    std::string expected = "File " + data_loc + fname + " empty.";

    REQUIRE_THROWS_MATCHES(
        [fname] {
            JsonLoader::from_file(data_loc + fname);
        }(),
        std::runtime_error, Catch::Matchers::Message(expected));
}

TEST_CASE("only allow strict (either object or array)", "[loader]") {

    SECTION("doesnt allow true") {
        std::string data = "true";
        std::string expected = R"(Load Error: json must be object or array
line: 1 position: 0
1:true
  ^~~~~~~~~~~
)";

        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }

    SECTION("doesnt allow false") {
        std::string data = "false";
        std::string expected = R"(Load Error: json must be object or array
line: 1 position: 0
1:false
  ^~~~~~~~~~~
)";

        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("doesnt allow nil") {
        std::string data = "nil";
        std::string expected = R"(Load Error: json must be object or array
line: 1 position: 0
1:nil
  ^~~~~~~~~~~
)";

        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("doesnt allow strings") {
        std::string data = "\"a\"";
        std::string expected = R"(Load Error: json must be object or array
line: 1 position: 0
1:"a"
  ^~~~~~~~~~~
)";

        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("doesnt allow numbers") {
        std::string data = "1337";
        std::string expected = R"(Load Error: json must be object or array
line: 1 position: 0
1:1337
  ^~~~~~~~~~~
)";

        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }

    SECTION("allows objects") {
        std::string data = "{}";
        REQUIRE_NOTHROW([data] {
            JsonLoader::from_string(data);
        }());
    }

    SECTION("allows arrays") {
        std::string data = "[]";
        REQUIRE_NOTHROW([data] {
            JsonLoader::from_string(data);
        }());
    }
}

TEST_CASE("no value", "[loader]") {
    std::string data = R"({
    "mama mia": {
        "nya": {
            "awa":
        },
    },
})";

    std::string expected = R"(Load Error: unexpected symbol for value
line: 5 position: 64
...
4:            "awa":
5:        },
          ^~~~~~~~~~~
)";

    REQUIRE_THROWS_MATCHES(
        [data] {
            JsonLoader::from_string(data);
        }(),
        JsonLoadErr, Catch::Matchers::Message(expected));
}

TEST_CASE("no key", "[loader]") {
    std::string data = R"({
    "mama mia": {
        "nya": {
            :"bab"
        },
    },
})";

    std::string expected =
        R"(Load Error: unexpected symbol, wanted key-value pair
line: 4 position: 49
...
3:        "nya": {
4:            :"bab"
              ^~~~~~~~~~~
)";

    REQUIRE_THROWS_MATCHES(
        [data] {
            JsonLoader::from_string(data);
        }(),
        JsonLoadErr, Catch::Matchers::Message(expected));
}

