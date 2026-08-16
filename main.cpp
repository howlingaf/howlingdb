
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include "src/buffer_pool_manager/BufferPoolManager.hpp"
#include "src/ingest/Parser.hpp"
#include "src/ingest/validate.hpp"

int main() {
  const std::string PATH_TO_CSV = "./example.csv";
  auto now = std::chrono::system_clock::now();
  std::cout << std::format("{:%F %T}", now) << '\n';
  std::ifstream file(PATH_TO_CSV);

  if (!file.is_open())
    return 1;
  const Result is_valid = validate(file);

  BufferPoolManager bpm;
  if (is_valid == SUCCESS) {
    std::cout << "Csv is valid" << '\n';
    ingest(file, bpm);
  }
  return 0;
}
