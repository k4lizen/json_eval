#include "expressions.hpp"
#include "err_matcher.hpp"
#include "json.hpp"

#include "catch_amalgamated.hpp"

using namespace k4json;

Json json = from_file("tests/data/a.json");

TEST_CASE("simple test", "[expression][syntax]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "$.mm['4']");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4);
    }());
}

TEST_CASE("quoted name selectors", "[expression][syntax]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "  mm   [\"key\"]   ['a']  ");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_bool() == true);

        result = parse(Json(result), "['keying a boolean']");
        REQUIRE(result.empty());

        result = parse(json, "$  ['⭐']");
        REQUIRE(result[0].get_string() == "⭐⭐");
    }());

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "['mm]");
        }(),
        ExprSyntaxErr,
        EqualsJError(5, "query ended early: unterminated name selector"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "[\"mm]\"");
        }(),
        ExprSyntaxErr,
        EqualsJError(6, "unterminated name selector, expected ]"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "[mm']");
        }(),
        ExprSyntaxErr, EqualsJError(3, "expected ]"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "[mm\"]");
        }(),
        ExprSyntaxErr, EqualsJError(3, "expected ]"));
}

TEST_CASE("dotted name selectors", "[expression][syntax]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "  mm   .key       .a  ");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_bool() == true);

        result = parse(Json(result), "keying_a_boolean");
        REQUIRE(result.empty());

        result = parse(json, "⭐");
        REQUIRE(result[0].get_string() == "⭐⭐");

        result = parse(json, "$.⭐");
        REQUIRE(result[0].get_string() == "⭐⭐");
    }());

    const std::string invalid_first =
        "invalid first character for dot-notation name selector";

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, " mm   .  key");
        }(),
        ExprSyntaxErr, EqualsJError(7, invalid_first));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "mm.4key");
        }(),
        ExprSyntaxErr, EqualsJError(3, invalid_first));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "mm.k\tey");
        }(),
        ExprSyntaxErr, EqualsJError(5, "expected end of expression"));
}
