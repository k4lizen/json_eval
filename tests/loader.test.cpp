#include "json.hpp"
#define CATCH_CONFIG_MAIN

#include "loader.hpp"
#include "catch_amalgamated.hpp"

std::string data_loc = "tests/data/";

// TODO: make a matching function so tests aren't so fickle

TEST_CASE("correctly desirializes", "[loader]") {
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
            "bnewline\n\fcc\rc\b\n\na\"quotes\"",
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
        Json j;
        SECTION ("string") {
            j = JsonLoader::from_string(data);
        }
        SECTION ("file") {
            j = JsonLoader::from_file(data_loc + "ok.json");
        }

        REQUIRE(j.size() == 2);
        Json mamamia = j["mama mia"];
        Json iscooking = j["is cooking"];
        REQUIRE(iscooking.get_string() == R"(something
Abbꪪ𐔁nya)");
        REQUIRE(mamamia.size() == 6);
        REQUIRE(mamamia["4"].get_number() == 4);
        REQUIRE(mamamia["this is"].get_string() == "pretty cool");
        REQUIRE(mamamia["nya"].size() == 1);
        REQUIRE(mamamia["nya"]["awa"].get_string() == "peko");
        REQUIRE(mamamia["key"].size() == 3);
        REQUIRE(mamamia["key"]["a"].get_bool() == true);
        REQUIRE(mamamia["key"]["b"].get_bool() == false);
        REQUIRE(mamamia["key"]["c"].is_null() == true);
        Json arr = mamamia["arr"];
        REQUIRE(arr.size() == 8);
        REQUIRE(arr[0].get_string() == "a");
        REQUIRE(arr[1].get_string() == "bnewline\n\fcc\rc\b\n\na\"quotes\"");
        // testing newline literally and escape codes as hex
        REQUIRE(arr[1].get_string() == std::string(R"(bnewline
)") + "\xc" "cc\xd" "c\x8" + R"(

a"quotes")");
        REQUIRE((arr[2].size() == 0 && arr[2].get_type() == JsonType::ARRAY));
        REQUIRE(arr[3].is_null() == true);
        REQUIRE(arr[4].get_bool() == true);
        REQUIRE(arr[5].get_number() == 3.13);
        REQUIRE((arr[6].size() == 1 && arr[6]["a"].size() == 0 && arr[6]["a"].get_type() == JsonType::OBJECT));
        REQUIRE((arr[7].size() == 2 && arr[7][0].get_string() == "a"));
        REQUIRE((arr[7][1].size() == 1 && arr[7][1]["b"][0].get_string() == "c"));
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

TEST_CASE("bad unicode", "[loader]") {
    SECTION("empty \\u") {
        std::string data = R"(["\u"])";
        std::string expected =
            R"(Load Error: the \uXXXX escape needs four characters
line: 1 position: 4
1:["\u"]
      ^~~~~~~~~~~
)";
        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("not enough \\uXXX") {
        std::string data = R"(["\u0BA","a"])";
        std::string expected =
            R"(Load Error: invalid hex character in \uXXXX escape sequence
line: 1 position: 7
1:["\u0BA","a"]
         ^~~~~~~~~~~
)";
        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("invalid hex character") {
        std::string data = R"(["\u1CGB"])";
        std::string expected =
            R"(Load Error: invalid hex character in \uXXXX escape sequence
line: 1 position: 6
1:["\u1CGB"]
        ^~~~~~~~~~~
)";
        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("unmatched low part of surrogate") {
        std::string data = R"(["\u0041\uDCBB"])";
        REQUIRE_THROWS_AS(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr);
    }
    SECTION("high part missing") {
        std::string data = R"(["\u0041\uD8ABcute"])";
        std::string expected =
            R"(Load Error: expected second (low) part of utf-16 surrogate pair
line: 1 position: 14
1:["\u0041\uD8ABcute"]
                ^~~~~~~~~~~
)";
        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
    SECTION("high part invalid") {
        std::string data = R"(["\u0041\uD8AB\uAAAA"])";
        std::string expected =
            R"(Load Error: codepoint isn't valid low part of utf-16 surrogate pair
line: 1 position: 16
1:["\u0041\uD8AB\uAAAA"]
                  ^~~~~~~~~~~
)";
        REQUIRE_THROWS_MATCHES(
            [data] {
                JsonLoader::from_string(data);
            }(),
            JsonLoadErr, Catch::Matchers::Message(expected));
    }
}

TEST_CASE("good unicode", "[loader]") {
    REQUIRE_NOTHROW([] {
        std::string data = R"(["A\u0042A", "𑚉𞠩𠁆💜š", "\uD805\uDe89\uD83A\uDC29\ud840\uDC46\uD83D\udc9C\u0161", "⦚\u299a\u299A"])";
        Json j = JsonLoader::from_string(data);
        REQUIRE(j[0].get_string() == "ABA");
        REQUIRE(j[1].get_string() == "𑚉𞠩𠁆💜š");
        REQUIRE(j[2].get_string() == "𑚉𞠩𠁆💜š");
        REQUIRE(j[3].get_string() == "⦚⦚\xE2\xA6\x9A");
    }());
}

TEST_CASE("good numbers", "[loader]") {

}

TEST_CASE("bad numbers", "[loader]") {

}

TEST_CASE("json type error", "[loader]") {

}
