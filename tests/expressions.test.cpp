#include "expressions.hpp"
#include "err_matcher.hpp"
#include "json.hpp"

#include "catch_amalgamated.hpp"

using namespace k4json;

Json json = from_file("tests/data/a.json");

TEST_CASE("simple test", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "$.mm['4']");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4);
    }());
}

TEST_CASE("quoted name selectors", "[expression]") {
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

TEST_CASE("dotted name selectors", "[expression]") {
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

TEST_CASE("expression in selector", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "mm[b][mm['4']]");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_bool() == true);

        result = parse(json, " mm [ $.b   ] [  mm   ['4']    + 1]");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 3.13);

        result = parse(json, "mm[$.b][min(mm['4']+1,size(mm[b]))]");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 3.13);

        result = parse(json, "mm[0]");
        REQUIRE(result.size() == 0);
    }());

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "mm[mm.arr]");
        }(),
        ExprValueErr,
        EqualsJError(
            9,
            R"(expression inside [...] must evaluate to [string] or [number], evaluates to:
[
  [
    "a",
    "bnewline\n\fcc\rc\b\n\na\"quotes\"",
    [ ],
    null,
    true,
    3.13,
    {
      "a": { }
    },
    [
      "a",
      {
        "b": [
          "c"
        ]
      }
    ]
  ]
])"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "mm[mm.arr.doesnt_exist]");
        }(),
        ExprValueErr,
        EqualsJError(22, "expression inside [...] must evaluate to one value, "
                         "evaluates to:\n[ ]"));
}

TEST_CASE("functions generic", "[expression]") {
    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(mm.arr.doesnt_exist)");
        }(),
        ExprValueErr,
        EqualsJError(24, "function argument cannot evaluate to nothing"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(mm.arr");
        }(),
        ExprSyntaxErr,
        EqualsJError(11, "function call unterminated, expected )"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(mm.arr, )");
        }(),
        ExprSyntaxErr,
        EqualsJError(13, "invalid first character in dot-notation name "
                         "selector or function name"));
}

TEST_CASE("min function", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "min(mm['4'],4)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4);

        result = parse(json, "min(mm['4'],3.99)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 3.99);

        result = parse(json, "min(mm['4'],4.01)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4.00);

        result = parse(json, "min(mm['4'], min(mm['4'], 15, 3))");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 3.00);

        std::string data = "[1, 2, 3]";
        result = parse(from_string(data), "min($)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 1);
    }());

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "min(4, 5, b, 7)");
        }(),
        ExprValueErr,
        EqualsJError(14, "function min() only accepts numerical arguments but "
                         "argument 2 is:\n\"arr\""));
}

TEST_CASE("max function", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "max(mm['4'],4)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4);

        result = parse(json, "max(mm['4'],3.99)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4);

        result = parse(json, "max(mm['4'],4.01)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4.01);

        result = parse(json, "max(mm['4'], max(mm['4'], 15, 3))");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 15);

        std::string data = "[1, 2, 3]";
        result = parse(from_string(data), "max($)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 3);
    }());

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "max(4, 5, b, 7)");
        }(),
        ExprValueErr,
        EqualsJError(14, "function max() only accepts numerical arguments but "
                         "argument 2 is:\n\"arr\""));
}

TEST_CASE("size function", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "size($)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 4);

        result = parse(json, "size(mm)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 6);

        result = parse(json, "size(mm.arr)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 8);

        result = parse(json, "size(b)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 3);
    }());

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(4)");
        }(),
        ExprValueErr,
        EqualsJError(7, "function size() is only valid for Json arrays, "
                        "objects and strings"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(mm['4'])");
        }(),
        ExprValueErr,
        EqualsJError(13, "function size() is only valid for Json arrays, "
                         "objects and strings"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(mm.key.a)");
        }(),
        ExprValueErr,
        EqualsJError(14, "function size() is only valid for Json arrays, "
                         "objects and strings"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "size(mm, mm, mm)");
        }(),
        ExprSyntaxErr,
        EqualsJError(16, "function size() only accepts one argument"));
}

TEST_CASE("nchildren function", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "nchildren($)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 28);

        result = parse(json, "nchildren(mm)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 24);

        result = parse(json, "nchildren(mm.arr)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 14);

        result = parse(json, "nchildren($, mm, mm.arr)");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 66);
    }());
}

TEST_CASE("binary operators", "[expression]") {
    REQUIRE_NOTHROW([] {
        JsonArray result = parse(json, "1 + 1");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 2);

        result = parse(json, "2 - 1");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 1);

        result = parse(json, "10 / 4");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 2.5);

        result = parse(json, "10 * 14.5");
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].get_number() == 145);

        result = parse(json, "10 + ((20 / 12) * 4) + (7 * 7)");
        REQUIRE(result.size() == 1);
        double diff = result[0].get_number() - 65.66666666666667;
        diff = diff >= 0 ? diff : -diff;
        REQUIRE(diff < 0.000001);
    }());

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "+");
        }(),
        ExprSyntaxErr, EqualsJError(0, "expected value, got operator"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "10 *");
        }(),
        ExprSyntaxErr, EqualsJError(4, "expected value"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "- 10");
        }(),
        ExprSyntaxErr, EqualsJError(0, "expected value, got operator"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "10 / b");
        }(),
        ExprValueErr,
        EqualsJError(6, "expression to the right of binary operator doesn't "
                        "resolve to [number]"));

    REQUIRE_THROWS_MATCHES(
        [] {
            parse(json, "mm / 10");
        }(),
        ExprValueErr,
        EqualsJError(3, "expression to the left of binary operator doesn't "
                        "resolve to [number]"));
}
