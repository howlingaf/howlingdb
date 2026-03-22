#include <array>
#include <fstream>
#include <iostream>
#include <string>
#include <variant>

struct Valid {};
struct Invalid {
  std::string reason;
};
using ValidatorResult = std::variant<Valid, Invalid>;

ValidatorResult is_csv_valid(std::string file); // return Valid, Invalid

// number of values should be less than or equal to number of headers for a
// given row if its greater its invalid
ValidatorResult is_csv_valid(std::ifstream &reader) {

  if (!reader.is_open()) {
    return Invalid{"File cannot be opened."};
  }

  if (reader.peek() == EOF) {
    return Invalid{"File is empty."};
  }

  std::array<char, 512> buffer;
  reader.read(buffer.data(), 512);
  for (std::streamsize i = 0; i < reader.gcount(); ++i) {
    // control chars 0 - 31 invalid, except \t tab, \n newline, \r carriage
    if (buffer[i] < 32 && buffer[i] != 9 && buffer[i] != 10 &&
        buffer[i] != 13) {
      return Invalid{"Invalid characters found (likely not text file)."};
    }
  }
  reader.clear();
  reader.seekg(0);
  std::string header;
  std::getline(reader, header);
  if (header.find(',') == std::string::npos) {
    return Invalid{"Header invalid - no commas found"};
  }
  return Valid{};
}
