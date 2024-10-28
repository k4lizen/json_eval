JSON processor (deserialize and query) written in C++

## Building
```
git clone https://github.com/k4lizen/json_eval.git
cd json_eval
make
```
## Usage
Example json file tests/data/simple.json:
```json
{
  "arr": [
    1, 2,
    "a", "b",
    {
      "c": [true, false, null]
    }
  ],
  "some string": "something\u000A\u0041bb\uAAAA\uD801\uDD01nya",
  "two": 2
}
```
Commands:
```
~> ./json_eval
usage: ./json_eval <json file> <query>

~> ./json_eval tests/data/simple.json "arr[two - 3]"
{
  "c": [
    true,
    false,
    null
  ]
}
~> ./json_eval tests/data/simple.json "arr[(two+6)/(2*size(arr[2]))]['c'][2]"
null
```
## Testing
```
make test
```
## Specification
Parsing of the input json file follows the latest RFC specification https://www.rfc-editor.org/rfc/rfc8259 .

Parsing of the query is a subset of https://www.rfc-editor.org/rfc/rfc9535 with certain extensions. The most notable restriction from the specification is that filter selectors are not supported.

### Extensions

The root identifier (`$`) can be ommited, `"abc.efg"` can be used as shorthand for `"$.abc.efg"`.

Everything is an expression. An expression can also be used as a selector `"arr[7 - 3]"` or even `"arr[arr[0]]"`.

Functions aren't selectors, they can be used freely in expressions `"min(4, size(arr))"`. The currently supported functions are `min`, `max` and `size`.

Binary operators are allowed in expressions e.g. `size(arr) + 3`, there are `+`, `-`, `*` and `/`. You can also use brackets `(` and `)` to enforce an order of operations other than left->right (important for expected behaviour of `*` and `/`!).

