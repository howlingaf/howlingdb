CXXFLAGS = --std=c++23 -g -fsanitize=address,undefined
howldb: *.cpp *.hpp
  g++ $(CXXFLAGS) *.cpp -o howlingdb
