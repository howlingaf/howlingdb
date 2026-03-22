// https://github.com/Aneurysm9/vm_challenge/blob/main/arch-spec
#include "is_csv_valid.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
// mmap does exactly that: it 'loads' the whole file into memory as a 1D array

int parse(std::ifstream &csv); // return Success or HasUnresolved

std::string PATH_TO_CSV = "./example.csv";

int main() {
  std::ifstream file(PATH_TO_CSV);
  // ValidatorResult is_valid = is_csv_valid(file);
  // if (std::holds_alternative<Invalid>(is_valid)) {
  //   std::cout << std::get<Invalid>(is_valid).reason << std::endl;
  // } else {
  //   std::cout << "Csv is valid" << std::endl;
  // }
  int parse_result = parse(file);
  return 0;
}

// push to db if row is valid,
int parse(std::ifstream &file) {
  int success_count;
  int failure_count;
  int runtime;
  std::cout << "Initiating parsing" << std::endl;
  bool IN_VALUE = false;
  std::vector<std::string> row;
  std::stringstream value;
  while (!file.eof()) {
    char curr = file.get();
    if (curr == '"') {
      if (!IN_VALUE) {
        if (value.tellp() == 0) {
          IN_VALUE = true;
        } else {
          value << curr;
        }
        continue;
      }
      //"ab"c"   "
      char next = file.peek();
      if (next == '\n' || next == ',') {
        IN_VALUE = false;
      } else if (next == '"') {
        value << curr;
        file.ignore();
      } else {
        throw std::runtime_error("Invalid Row");
      }
    } else if (curr == ',' && !IN_VALUE) {
      row.push_back(value.str());
      value = std::stringstream();
    } else if (curr == '\n' && !IN_VALUE) {
      row.push_back(value.str());
      value = std::stringstream();
      for (auto &i : row) {
        std::cout << i + "|";
      }
      std::cout << std::endl;
      row.clear();
    }
    // any other character
    else {
      value << curr;
    }
  }
  return 0;
}
