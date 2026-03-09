#include <fstream>
#include <iostream>
#include <string>
// define what a csv engine is and what it does
// mmap does exactly that: it 'loads' the whole file into memory as a 1D array
std::string line;

// if you have void parse it means you can call it any number of times with the
// same instance of CSVObject which makes zero sense
class CSVObject {                 // return CSVDocument?
  std::ifstream normalized_input; // reference to disk (read mmap and laziness)
public:
  void parse(std::ifstream raw) {
    if (!raw.is_open()) {
      std::cout << "File could not be opened" << std::endl;
    };

    while (std::getline(raw, line)) {
      std::cout << line << std::endl;
    }
  }
};

// row members members, headers,rows behavior
// init
// contruct
// read and normalize raw
// write to disk
// get setters

int main() { return 0; }
