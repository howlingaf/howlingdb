#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <unordered_map>

#include "BufferPoolManager.hpp"

#include "types.hpp"

std::unordered_map<int, std::string> error_code_to_message = {
    {1, "Invalid Csv."},
};

std::string PATH_TO_CSV = "./example.csv";

int main() {
  auto now = std::chrono::system_clock::now();
  std::cout << std::format("{:%F %T}", now) << '\n';
  std::ifstream file(PATH_TO_CSV);

  if (!file.is_open()) {
    return 1;
  };

  BufferPoolManager buffer_manager;
  ValidatorResult is_valid = validate(file);
  if (std::holds_alternative<Valid>(is_valid)) {
    std::cout << "Csv is valid" << '\n';
    buffer_manager.ingest(file);
  }

  if (std::holds_alternative<Invalid>(is_valid)) {
    std::print("{} {{row:{}, col:{}}}", std::get<Invalid>(is_valid).reason,
               std::get<Invalid>(is_valid).error_row,
               std::get<Invalid>(is_valid).error_col);
  }

  return 0;
}
