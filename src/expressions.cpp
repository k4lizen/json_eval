#include "expressions.hpp"
#include "json.hpp"
#include "utils.hpp"
#include <cassert>
#include <charconv>
#include <cmath>

JsonExpressionParser::JsonExpressionParser(const Json& json,
                                           const std::string& expression) {
    // As per the spec
    // https://www.rfc-editor.org/rfc/rfc9535#name-json-values-as-trees-of-nod
    // we will model the result of a query as a nodelist
    this->rootlist = JsonArray();
    this->rootlist.push_back(json);
    this->buffer = expression;
}

JsonArray JsonExpressionParser::parse(const Json& json,
                                      const std::string& expression) {
    JsonExpressionParser jep(json, expression);
    return jep.parse();
}

// Advance only if the next character matches param
bool JsonExpressionParser::match(const char c) {
    if (peek() == c) {
        next();
        return true;
    }
    return false;
}

// TODO: is this good design?
void JsonExpressionParser::assert_match(const char c) {
    assert(peek() == c);
    next();
}

// Return current character
char JsonExpressionParser::peek() {
    if (reached_end()) {
        return '\0';
    }

    return buffer[current];
}

char JsonExpressionParser::peekNext() {
    if (current + 1 >= buffer.size()) {
        return '\0';
    }

    return buffer[current + 1];
}

// TODO: make same definition in loader.cpp? valid there?
// Advance and return next character
char JsonExpressionParser::next() {
    if (current + 1 >= buffer.size()) {
        if (current < buffer.size()) {
            current++;
        }
        return '\0';
    }

    return buffer[++current];
}

bool JsonExpressionParser::reached_end() {
    return current >= buffer.size();
}

// Skip all whitespace
void JsonExpressionParser::skip() {
    // bounds checking is happening in peek() and next()
    char c = peek();
    while (is_whitespace(c)) {
        if (c == '\n') {
            line++;
        }
        c = next();
    }
}

void JsonExpressionParser::expr_err(const std::string& msg) {
    std::string res = "Json Expression Error: " + msg + '\n';
    res += buffer + '\n';
    res += pretty_error_pointer(current);
    throw JsonExprErr(res);
}

JsonArray JsonExpressionParser::parse_max() {
    exit(1);
}
JsonArray JsonExpressionParser::parse_min() {
    exit(1);
}
JsonArray JsonExpressionParser::parse_size() {
    exit(1);
}

JsonArray JsonExpressionParser::parse_func(FuncType func) {
    assert_match('(');
    switch (func) {
    case FuncType::MAX:
        return parse_max();
    case FuncType::MIN:
        return parse_min();
    case FuncType::SIZE:
        return parse_size();
    default:
        assert(0); // TODO: this is ugly
    }
}

FuncType string_to_functype(std::string_view sv) {
    if (sv == "min") {
        return FuncType::MIN;
    } else if (sv == "max") {
        return FuncType::MAX;
    } else if (sv == "size") {
        return FuncType::SIZE;
    } else {
        return FuncType::INVALID;
    }
}

// For situations like [a.b[1]]
// Number literals also count as expressions: [7]
JsonArray JsonExpressionParser::parse_expr_selector(
    const JsonArray& nodelist) {
    assert_match('[');

    // Weirdness due to the the spec extension coming from two facts:
    // 1. We allow "a" as a valid path, no need for "$.a"
    // 2. We allow expressions inside [ ]
    // Json:
    // {
    //     "0": 1,
    //     "arr": [
    //             "as index",
    //             "as expression"
    //     ]
    // }
    // Should the query: "arr[0]" return ["as index"] or ["as expression"]?
    // Luckily digits aren't allowed as the first character for dot-notation
    // names, so we always interpret digits like literals (as index) in cases
    // like this.

    // TODO: do I want multi-indexing?

    JsonArray inside = parse_inner();

    skip();
    if (!match(']')) {
        expr_err("expected ]");
    }

    // The expression needs to evaluate to a one-element array
    // containing either an integer or a string
    if (inside.size() != 1) {
        expr_err("expression inside [...] must evaluate to one value, "
                 "evaluates to: " +
                 Json(inside).to_string());
    }

    if (inside[0].get_type() == JsonType::STRING) {
        return parse_name(nodelist, inside[0].get_string());
    }

    if (inside[0].get_type() != JsonType::NUMBER) {
        expr_err("expression inside [...] must evaluate to [string] or [number], evaluates to: " + Json(inside).to_string());
    }

    double number = inside[0].get_number();
    if (std::floor(number) != number) {
        expr_err("expression inside [...] evaluates to number (" + std::to_string(number) + "), but not an integer");
    }

    // https://www.rfc-editor.org/rfc/rfc9535#name-semantics-5
    int idx = static_cast<int>(number);
    JsonArray res;
    for (auto& node : nodelist) {
        // Nothing on non-arrays
        if (node.get_type() != JsonType::ARRAY) {
            continue;
        }
        int cidx = idx;
        // We need to accept negative numbers
        if (cidx < 0) {
            cidx = node.size() + cidx;
        }
        // Nothing on out of bounds
        if (cidx < 0 || cidx >= node.size()) {
            continue;
        }
        res.push_back(node[cidx]);
    }
    
    return res;
}

JsonArray JsonExpressionParser::parse_name(const JsonArray& nodelist,
                                           std::string_view name) const {
    JsonArray res;
    for (auto& node : nodelist) {
        if (node.get_type() == JsonType::OBJECT && node.obj_contains(name)) {
            res.push_back(node[name]);
        }
    }
    return res;
}

bool valid_dot_name_first(unsigned char c) {
    // name-first          = ALPHA /
    //                       "_"   /
    //                       %x80-D7FF /
    //                          ; skip surrogate code points
    //                       %xE000-10FFFF
    // ALPHA               = %x41-5A / %x61-7A    ; A-Z / a-z

    // Since we are assuming our input is in UTF-8 we don't
    // need to check for surrogates.
    // We allow all UTF-8 bytes with the (c > 127) check.
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' ||
           c > 127;
}

bool valid_dot_name_char(unsigned char c) {
    // name-char           = name-first / DIGIT
    // DIGIT               = %x30-39              ; 0-9

    return valid_dot_name_first(c) || ('0' <= c && c <= '9');
}

bool valid_dot_notation_name(std::string_view name) {
    // https://www.rfc-editor.org/rfc/rfc9535#section-2.5.1.1
    // member-name-shorthand = name-first *name-char

    if (name.empty())
        return false;

    if (!valid_dot_name_first(name[0])) {
        return false;
    }

    for (unsigned int i = 1; i < name.size(); ++i) {
        if (!valid_dot_name_char(name[i])) {
            return false;
        }
    }
    return true;
}

JsonArray
JsonExpressionParser::parse_name_selector_dotted(const JsonArray& nodelist) {
    assert_match('.');

    int start = current;
    char c = peek();
    if (!valid_dot_name_first(c)) {
        expr_err("invalid first character for dot-notation name selector");
    }

    while (valid_dot_name_char(c)) {
        c = next(); // bounds checking is implicit
    }

    return parse_name(nodelist,
                      std::string_view(buffer).substr(start, current - start));
}

JsonArray
JsonExpressionParser::parse_name_selector_quoted(const JsonArray& nodelist,
                                                 char quote) {
    assert_match('[');
    assert_match(quote);

    int start = current;
    char c = peek();
    while (!reached_end() && c != quote) {
        c = next();
    }

    // either the quote or the ] isn't closed
    if (reached_end()) {
        expr_err("query ended early: unterminated name selector");
    }

    std::string_view name =
        std::string_view(buffer).substr(start, current - start);
    assert_match(quote);
    if (!match(']')) {
        expr_err("unterminated name selector, expected ]");
    }
    return parse_name(nodelist, name);
}

JsonArray JsonExpressionParser::parse_selector(const JsonArray& nodelist) {
    if (nodelist.empty()) {
        return JsonArray();
    }

    // I) We have three valid selectors inside brackets:
    // 1. (single or double) quote escaped: ["some field"]; ['some field']
    //     denoting an object key
    // 2. integer: [10]
    //     denoting an array index
    // 3. expression (non-rfc extension): [a.b["c"]]
    //     which evaluates to one 1. or 2.

    // II) We also have the dot-shorthand: .something
    //     denoting an object key

    // No error should be produced when applying indexing to objects
    // or keying to arrays, simply output nothing.
    // See https://www.rfc-editor.org/rfc/rfc9535#name-semantics-3
    // and https://www.rfc-editor.org/rfc/rfc9535#name-semantics-5

    char p = peek();
    assert(p == '[' || p == '.');
    char pn = peekNext();
    if (p == '[') {
        // I)
        if (pn == '\'' || pn == '"') {
            // I) 1.
            return parse_name_selector_quoted(nodelist, pn);
        } else {
            // I) 2. && 3.
            return parse_expr_selector(nodelist);
        }
    } else {
        // II)
        return parse_name_selector_dotted(nodelist);
    }
}

JsonArray JsonExpressionParser::parse_path(std::string_view obj_beginning) {
    char c = peek();
    assert(c == '.' || c == '[');

    JsonArray res = rootlist;
    if (obj_beginning != "") {
        // We need to parse this before we continue with this->current.
        // Doing it this way is an optimization circumventing the fact that
        // we needed to figure out whether this was a function call or
        // a path expression.
        res = parse_name(res, obj_beginning);
    }

    while (!reached_end() && (c == '.' || c == '[')) {
        // TODO: a copy happens here (trust), how to optimize?
        // this->current gets advanced inside \/
        res = parse_selector(res);
        skip();
        c = peek();
    }

    return res;
}

JsonArray JsonExpressionParser::parse_func_or_path() {
    char c;
    // $ means we are for sure in a path
    if (match('$')) {
        // jsonpath-query      = root-identifier segments
        // segments            = *(S segment)
        skip();
        if (reached_end()) {
            return rootlist;
        }
        c = peek();
        if (c != '.' && c != '[') {
            expr_err("invalid character, expected . or [");
        }

        return parse_path("");
    }
    // Posibilities:
    // [
    //     spec segment
    // something(
    //     function call
    // something.
    // something[
    //     dot-notation name selector
    //     (without dot since it's at the beginning of the path)
    //     (this is our extension, not in the spec)
    // something<end of string>
    //     same as above
    // something+ (et al.)
    //     same as above

    // TODO:
    // something +, something [, something .
    // aren't allowed but they should be

    c = peek();
    if (c == '[') {
        return parse_path("");
    }

    int start = current;
    // allowed function name characters are a subset of allowed dot name
    // characters
    if (!valid_dot_name_first(c)) {
        expr_err("invalid first character in dot-notation name selector or "
                 "function name");
    }
    c = next();

    while (!reached_end()) {
        switch (c) {
        case '(': {
            // something( function
            std::string_view sv =
                std::string_view(buffer).substr(start, current - start);
            FuncType func = string_to_functype(sv);
            if (func == FuncType::INVALID) {
                expr_err("invalid function name '" + std::string(sv) + "'");
            }
            return parse_func(func);
        }
        case '.':
        case '[':
            // something. or something[ path
            return parse_path(std::string_view(buffer).substr(
                                            start, current - start));
        case '+':
        case '-':
        case '*':
        case '/':
            return parse_name(rootlist, std::string_view(buffer).substr(
                                            start, current - start));
        default:
            if (!valid_dot_name_char(c)) {
                expr_err("invalid character in dot-notation name selector or "
                         "function name");
            }
        }

        c = next();
    }

    // something<end of string>
    return parse_name(rootlist,
                      std::string_view(buffer).substr(start, current - start));
}

// TODO: deduplicate??
// If false is returned, number is undefined
bool JsonExpressionParser::match_number(double& number) {
    const char* cbuff = buffer.c_str();
    auto [ptr, ec] =
        std::from_chars(cbuff + current, cbuff + buffer.size() - 1, number);

    if (ec == std::errc::invalid_argument) {
        // no number at that location
        return false;
    }

    if (ec == std::errc::result_out_of_range) {
        expr_err("number out of range");
    }

    // valid number!
    current = ptr - cbuff;
    return true;
}

bool is_binary_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

// Returns error code:
// 0 - no error
// 1 - division by zero
int apply_operator(double& result, double operand, Operator operation) {
    switch (operation) {
    case Operator::NONE:
        result = operand;
        return 0;
    case Operator::PLUS:
        result += operand;
        return 0;
    case Operator::MINUS:
        result -= operand;
        return 0;
    case Operator::MUL:
        result *= operand;
        return 0;
    case Operator::DIV:
        if (operand == 0) {
            return 1;
        }
        result /= operand;
        return 0;
    }
}

// Can be a subexpression
JsonArray JsonExpressionParser::parse_inner() {
    // The constructs we encounter here go to either
    // 1. match_number
    // 2. + - / * processors
    // 3. parse_func_or_path

    // The + - / * operators can only operate on numbers,
    // so we will keep an accumulative value for that case to save
    // on overhead from putting / extracting numbers to JsonArray

    // Whether the expression can end here
    // (in terms of the binary operators)
    bool expecting = true;
    // Only valid if (expecting == true)
    Operator last_op = Operator::NONE;
    double num_total = 0;
    // In case the expression doesn't use operators at all
    JsonArray res;
    bool first_is_non_numeric = false;

    while(!reached_end()) {
        skip();

        // If a number is matched it couldn't have been a valid func/path/operator
        // the - operator 
        double number;
        if (match_number(number)) {
            if (!expecting) {
                // x-y interpreted as x -y instead of x - y
                if (number < 0) {
                    num_total -= number;
                    continue;
                }
                
                expr_err("expecting end of expression or binary operator, got number");
            }

            if (apply_operator(num_total, number, last_op) == 1){
                expr_err("division by zero");
            }

            expecting = false;
            continue;
        }

        char c = peek();
        // If an operator is matched it couldn't have been a valid func/path
        if (is_binary_operator(c)) {
            if (expecting) {
                expr_err("expected value, got operator");
            }

            if (first_is_non_numeric) {
                expr_err("expression to the left of binary operator doesn't resolve to [number]");
            }

            switch (c) {
            case '+':
                last_op = Operator::PLUS;
                break;
            case '-':
                last_op = Operator::MINUS;
                break;
            case '*':
                last_op = Operator::MUL;
                break;
            case '/':
                last_op = Operator::DIV;
                break;
            }
            expecting = true;
            next();
            continue;
        }

        if (!expecting) {
            break;
        }

        JsonArray cur = parse_func_or_path();
        if (cur.size() == 1 && cur[0].get_type() == JsonType::NUMBER) {
            if (apply_operator(num_total, cur[0].get_number(), last_op) == 1){
                expr_err("division by zero");
            }
        } else {
            if (last_op == Operator::NONE) {
                res = std::move(cur);
                first_is_non_numeric = true;
            } else {
                expr_err ("expression to the right of binary operator doesn't resolve to [number]");
            }
        }
        expecting = false;
    }

    if (first_is_non_numeric) {
        return res;
    }

    res.push_back(Json(num_total));
    return res;
}

// The user supplied expression
JsonArray JsonExpressionParser::parse() {
    current = 0;
    line = 1;

    skip();
    if (reached_end()) {
        return rootlist;
    }

    JsonArray res = parse_inner();

    skip();
    if (!reached_end()) {
        expr_err("expected end of expression");
    }

    return res;
}
