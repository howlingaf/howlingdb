#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include "src/buffer_pool_manager/BufferPoolManager.hpp"
#include "src/ingest/validate.hpp"

const std::string PATH_TO_CSV = "./example.csv";

int main() {
  auto now = std::chrono::system_clock::now();
  std::cout << std::format("{:%F %T}", now) << '\n';
  std::ifstream file(PATH_TO_CSV);

  if (!file.is_open()) {
    return 1;
  };
  BufferPoolManager manager;
  const Result is_valid = validate(file);
  if (is_valid == SUCCESS) {
    std::cout << "Csv is valid" << '\n';
    manager.ingest(file);
  }
  return 0;
}
