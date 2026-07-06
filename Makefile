CXXFLAGS = --std=c++23 -g -fsanitize=address,undefined -Wall -Wextra -Wconversion -Werror 
howldb: $(wildcard *.cpp) $(wildcard src/*/*.hpp)
	g++ $(CXXFLAGS) *.cpp -o howlingdb
