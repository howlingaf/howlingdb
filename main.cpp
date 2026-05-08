#include <chrono>
#include <format> // Requires C++20
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "DiskManager.hpp"
#include "Ingest.hpp"
#include "PageManager.hpp"

#include "types.hpp"

std::unordered_map<int, std::string> error_code_to_message = {
    {1, "Invalid Csv."},
};

std::string PATH_TO_CSV = "./example.csv";

int main() {
  auto now = std::chrono::system_clock::now();
  std::cout << std::format("{:%F %T}", now) << std::endl;
  std::ifstream file(PATH_TO_CSV);

  if (!file.is_open()) {
    throw std::runtime_error{"File could not be opened"};
  };
  DiskManager disk_manager;
  PageManager page_manager(disk_manager);
  Ingest ingest(page_manager);
  ValidatorResult is_valid = validate(file);
  if (std::holds_alternative<Valid>(is_valid)) {
    std::cout << "Csv is valid" << std::endl;
    IngestResult result = ingest.parse(file);
  }

  else if (std::holds_alternative<Invalid>(is_valid)) {
    printf("%s {row:%d,col:%d}", std::get<Invalid>(is_valid).reason.c_str(),
           std::get<Invalid>(is_valid).error_row,
           std::get<Invalid>(is_valid).error_row);
  }

  return 0;
}
