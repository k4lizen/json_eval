JSON processor (deserialize and query) written in C++

## Building
```
git clone https://github.com/k4lizen/json_eval.git
cd json_eval
make
```
## Usage
```
~> ./json_eval
usage: ./json_eval <json file> <query>
TODO: add real example from tests/data/
~> ./json_eval test.json "a.b[1]"
2
```
## Testing
```
make test
```
## Specification
Parsing of the input json file follows the latest RFC specification https://www.rfc-editor.org/rfc/rfc8259 .

Parsing of the query is a subset of https://www.rfc-editor.org/rfc/rfc9535 with certain extensions.
