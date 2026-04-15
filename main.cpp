#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "parse.cpp"

std::unordered_map<int, std::string> error_code_to_message = {
    {1, "Invalid Csv."},
};

std::string PATH_TO_CSV = "./test/customers-10000.csv";

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
