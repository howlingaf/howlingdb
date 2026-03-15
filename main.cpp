#include <fstream>
#include <iostream>
#include <string>
#include <vector>
// define what a csv engine is and what it does
// mmap does exactly that: it 'loads' the whole file into memory as a 1D array

void is_valid_csv(std::ifstream &csv); // return Valid, Invalid
void parse(std::ifstream &csv);        // return Success or HasUnresolved

std::string PATH_TO_CSV = "./example.csv";
int main() {
  std::ifstream csv(PATH_TO_CSV);
  is_valid_csv(csv);

  return 0;
}

void is_valid_csv(std::ifstream &file) {
  if (!file.is_open()) {
    std::cout << "File could not be opened" << std::endl;
  };
  if ()
    std::string line;

  // file extension is not csv
  //  file is not empty
  //  check first line for binary content (null or non-text chars fail)
  //  check first line parses as header
  while (std::getline(file, line)) {
    std::cout << line << std::endl;
  }
  // assert file is csv
}

void parse(std::ifstream &file) {
  // parse -> returns enum with resolved/unresolved writes, most common error
  // code, time, etc
  // parse hands off to resolved db or error db
}
