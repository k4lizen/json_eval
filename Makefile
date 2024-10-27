project := json_eval

CXX := g++
CXXFLAGS := -std=c++20 -Iexternal -Isrc -Wall -Wextra -g

objects := main.o expressions.o generic_parser.o json.o loader.o utils.o
objects := $(addprefix build/, $(objects))

test_objects := err_matcher.o expressions.test.o json.test.o loader.test.o
test_objects := $(addprefix build/tests/, $(test_objects))

all: $(project)

$(project): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(project) $^

build/%.o: src/%.cpp | build_dir
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $^

build_dir:
	mkdir -p build

test: runtests
	./runtests -i

runtests: $(objects) $(test_objects) build/catch_amalgamated.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(filter-out build/main.o, $^)

build/tests/%.o: tests/%.cpp | test_build_dir
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $^

test_build_dir:
	mkdir -p build/tests
	
build/catch_amalgamated.o: external/catch_amalgamated.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c -o $@ $^
	
clean:
	rm -r build/*
