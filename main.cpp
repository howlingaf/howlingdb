#include "is_csv_valid.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <variant>
// mmap does exactly that: it 'loads' the whole file into memory as a 1D array

struct ParseResult {
  int success_count;
  int failure_count;
  int runtime;
};

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

// push to db if row is valid,
ParseResult parse(std::ifstream &file) {
  int success_count;
  int failure_count;
  int runtime;
  std::cout << "Initiating parsing" << std::endl;
  return ParseResult{success_count, failure_count, runtime};
}
