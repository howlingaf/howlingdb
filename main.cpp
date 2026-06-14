#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <unordered_map>
#include <variant>

#include "src/buffer_pool_manager/BufferPoolManager.hpp"

enum State : int8_t {
  Success,
  PartialSuccess,
  Failure,
};

struct Valid {};

struct Invalid {
  std::string reason;
  int error_row;
  int error_col;
};

using ValidatorResult = std::variant<Valid, Invalid>;

ValidatorResult validate(std::ifstream &reader);

const std::unordered_map<int, std::string> error_code_to_message = {
    {1, "Invalid Csv."},
};

const std::string PATH_TO_CSV = "./example.csv";

int main() {
  auto now = std::chrono::system_clock::now();
  std::cout << std::format("{:%F %T}", now) << '\n';
  std::ifstream file(PATH_TO_CSV);

  if (!file.is_open()) {
    return 1;
  };

  BufferPoolManager manager;
  ValidatorResult is_valid = validate(file);
  if (std::holds_alternative<Valid>(is_valid)) {
    std::cout << "Csv is valid" << '\n';
    manager.ingest(file);
  }

  if (std::holds_alternative<Invalid>(is_valid)) {
    std::print("{} {{row:{}, col:{}}}", std::get<Invalid>(is_valid).reason,
               std::get<Invalid>(is_valid).error_row,
               std::get<Invalid>(is_valid).error_col);
  }

  return 0;
}
