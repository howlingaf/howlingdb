CXXFLAGS = --std=c++23 -Isrc -g -fsanitize=address,undefined -Wall -Wextra -Wconversion -Werror 
howldb: $(wildcard *.cpp) $(wildcard src/*/*.hpp)
	g++ $(CXXFLAGS) *.cpp -o howlingdb

page_test: tests/page_test.cpp $(wildcard src/*/*.hpp)
	g++ $(CXXFLAGS) tests/page_test.cpp -o page_test

test: page_test
	./page_test

.PHONY: test
