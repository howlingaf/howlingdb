CXXFLAGS = --std=c++23 -g -fsanitize=address,undefined
howldb: $(wildcard *.cpp) $(wildcard src/*/*.hpp)
	g++ $(CXXFLAGS) *.cpp -o howlingdb
