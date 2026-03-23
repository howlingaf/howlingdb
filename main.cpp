// https://github.com/Aneurysm9/vm_challenge/blob/main/arch-spec
#include "is_csv_valid.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
// MMAP
// Do we want a validation pass or do we want to validate and push as we go
// Error that should halt program (suceeding fields may be malformed)
// Row error that can be safely skipped

enum State {
  Success,
  PartialSuccess,
  Failure,
};

std::unordered_map<int, std::string> error_code_to_message = {
    {1, "Invalid Csv."},
};

struct ParseResult {
  State state;
  std::vector<std::string> invalid_rows;
  int error_code; // need some mapping code->error message
};

ParseResult parse(std::ifstream &csv); // return Success or HasUnresolved

std::string PATH_TO_CSV = "customers-10000.csv";

int main() {
  std::ifstream file(PATH_TO_CSV);

  if (!file.is_open()) {
    throw std::runtime_error{"File could not be opened"};
  };

  ValidatorResult is_valid = is_csv_valid(file);
  if (std::holds_alternative<Valid>(is_valid)) {
    std::cout << "Csv is valid" << std::endl;
    ParseResult parse_result = parse(file);
  }

  else if (std::holds_alternative<Invalid>(is_valid)) {
    printf("%s {row:%d,col:%d}", std::get<Invalid>(is_valid).reason.c_str(),
           std::get<Invalid>(is_valid).error_row,
           std::get<Invalid>(is_valid).error_row);
  }

  return 0;
}

// push to db if row is valid,
ParseResult parse(std::ifstream &reader) {
  int success_count;
  int failure_count;
  int runtime;
  std::cout << "Initiating parsing" << std::endl;
  bool IN_VALUE = false;
  std::vector<std::string> row;
  std::stringstream value;
  while (!reader.eof()) {
    char curr = reader.get();
    if (curr == '"') {
      if (!IN_VALUE) {
        if (reader.tellg() == 0) {
          printf("%d\n", (int)reader.tellg());
          IN_VALUE = true;
        } else {
          value << curr;
        }
        continue;
      }
      char next = reader.peek();
      if (next == '\n' || next == ',') {
        IN_VALUE = false;
      } else if (next == '"') {
        value << curr;
        reader.ignore();
      } else {
        return ParseResult{
            Failure,
            {},
            1,
        };
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
    } else {
      value << curr;
    }
  }
  return ParseResult{Success, {}, -1};
}
