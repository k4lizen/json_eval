#include "parser.hpp"
#include "utils.hpp"
#include <cassert>
#include <charconv>

namespace k4json {

// Advance only if the next character matches param
bool Parser::match(const char c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

// TODO: is this good design?
void Parser::assert_match(const char c) {
    assert(peek() == c);
    next();
}

// Return current character
char Parser::peek() {
    if (reached_end()) {
        return '\0';
    }

    return buffer[current];
}

char Parser::peekNext() {
    if (current + 1 >= buffer.size()) {
        return '\0';
    }

    return buffer[current + 1];
}

// Advance and return next character
char Parser::next() {
    if (current + 1 >= buffer.size()) {
        if (current < buffer.size()) {
            current++;
        }
        return '\0';
    }

    return buffer[++current];
}

bool Parser::reached_end() {
    return current >= buffer.size();
}

// Skip all whitespace
void Parser::skip() {
    // bounds checking is happening in peek() and next()
    char c = peek();
    while (is_whitespace(c)) {
        if (c == '\n') {
            line++;
        }
        c = next();
    }
}

// If false is returned, number is undefined
bool Parser::match_number(double& number) {
    // We are strictly conforming to:
    // https://www.rfc-editor.org/rfc/rfc8259#section-6

    // > Since software that implements
    // > IEEE 754 binary64 (double precision) numbers [IEEE754] is generally
    // > available and widely used, good interoperability can be achieved by
    // > implementations that expect no more precision or range than these
    // > provide, [...]
    //
    // So we will use a double (64 bits) for the number type

    // We can't use strtod and friends for the conversion because they
    // don't conform to the json definition of a number, but from_chars
    // *mostly* does. Also, see Notes section in
    // https://en.cppreference.com/w/cpp/utility/from_chars

    const char* cbuff = buffer.c_str();
    // We give it the whole rest of the buffer, it will only care about the
    // initial valid part
    auto [ptr, ec] =
        std::from_chars(cbuff + current, cbuff + buffer.size() - 1, number);
    if (ec == std::errc::invalid_argument) {
        // no number at that location
        return false;
    }
    if (ec == std::errc::result_out_of_range) {
        syntax_err("number out of range");
        return false;
    }

    // Checks that from_chars doesn't perform
    std::string_view numstr = std::string_view(buffer).substr(current, ptr - (current + cbuff));
    // leading zeroes aren't allowed
    if (numstr[0] == '0' && numstr != "0") {
        syntax_err("number cannot have leading zeroes");
        return false;
    }
    if (numstr.size() > 1 && numstr[0] == '-' && numstr[1] == '0' && numstr != "-0") {
        syntax_err("(negative) number cannot have leading zeroes");
        return false;
    }
    // omitting the integer part in a fraction (.123 like 0.123) isn't allowed
    if (numstr[0] == '.' || (numstr.size() > 1 && numstr[0] == '-' && numstr[1] == '.')) {
        syntax_err("fractional number must have integer component");
        return false;
    }

    // valid number!
    current = ptr - cbuff;
    return true;
}

} // namespace k4json
