#include "json.hpp"
#include "err_matcher.hpp"

#include "catch_amalgamated.hpp"

TEST_CASE("json type error", "[json]") {
    std::string data = "{ \"a\": [1, \"abc\"] }";

    Json j = Json::from_string(data);

    REQUIRE_THROWS_MATCHES(
        [j] {
            j.get_number();
        }(),
        JsonTypeErr,
        EqualsJError(
            "get_number() called on Json which isnt JsonType::NUMBER"));

    REQUIRE_THROWS_MATCHES(
        [j] {
            j.get_string();
        }(),
        JsonTypeErr,
        EqualsJError(
            "get_string() called on Json which isnt JsonType::STRING"));

    REQUIRE_THROWS_MATCHES(
        [j] {
            j.get_array();
        }(),
        JsonTypeErr,
        EqualsJError("get_array() called on Json which isnt JsonType::ARRAY"));

    REQUIRE_THROWS_MATCHES(
        [j] {
            j.get_bool();
        }(),
        JsonTypeErr,
        EqualsJError("get_bool() called on Json which isnt JsonType::BOOL"));

    REQUIRE_THROWS_MATCHES(
        [j] {
            j["a"]["b"];
        }(),
        JsonTypeErr,
        EqualsJError("operator[std::string] invalid, instance isn't JsonType::OBJECT"));

    REQUIRE_NOTHROW([j] {
        REQUIRE(j["a"][1].get_string() == "abc");
    }());
}
