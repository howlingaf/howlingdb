#include "is_csv_valid.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <variant>
#include <vector>
// mmap does exactly that: it 'loads' the whole file into memory as a 1D array

struct Resolved {};
struct Unresolved {
  std::vector<std::string> unresolved;
};

using ParseResult = std::variant<Resolved, Unresolved>;
ParseResult parse(std::ifstream &csv); // return Success or HasUnresolved

std::string PATH_TO_CSV = "./example.csv";

int main() {
  std::ifstream file(PATH_TO_CSV);
  ValidatorResult is_valid = is_csv_valid(PATH_TO_CSV, file);
  if (std::holds_alternative<Invalid>(is_valid)) {
    std::cout << std::get<Invalid>(is_valid).reason << std::endl;
  } else {
    std::cout << "Csv is valid" << std::endl;
  }
  ParseResult parse_result = parse(file);
  return 0;
}

ParseResult parse(std::ifstream &file) {
  std::cout << "Initiating parsing" << std::endl;
  // parse -> returns enum with resolved/unresolved writes, most common error
  // code, time, etc
  // parse hands off to resolved db or error db
  return Resolved{};
}
