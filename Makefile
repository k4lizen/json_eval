project := json_eval

CXX := g++
CXXFLAGS := -std=c++20 -Iexternal -Isrc -Wall -Wextra -g

srcfiles := $(wildcard src/*.cpp)
testfiles := $(wildcard tests/*.cpp)

objects  := $(patsubst %.cpp, %.o, $(srcfiles))
testobjects := $(patsubst %.cpp, %.o, $(testfiles))

all: $(project)

$(project): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(project) $(objects) $(LDLIBS)

test: $(testobjects) $(objects)
	$(CXX) $(CXXFLAGS) -o runtests $(testobjects) $(filter-out src/main.o,$(objects)) external/catch_amalgamated.cpp $(LDLIBS)
	./runtests -i

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^ >>./.depend;

clean:
	rm -f $(objects) $(testobjects)

include .depend
