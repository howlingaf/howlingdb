#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <variant>

struct Valid {};
struct Invalid {
  std::string reason;
};
using ValidatorResult = std::variant<Valid, Invalid>;

ValidatorResult is_csv_valid(std::ifstream &reader) {

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
  bool in_quote = false;
  bool field_start = true;

  while (!reader.eof()) {
    char curr = reader.get();
    if (in_quote) {
      if (curr == '"') {
        char next = reader.peek();
        if (next == '"') {
          reader.ignore();
        } else if (next == '\n' || next == ',') {
          in_quote = false;
        } else {
          return Invalid{"Ambiguity found with quote due to double quotes"};
        }
      }
    } else {
      if (curr == '"' && field_start) {
        in_quote = true;
      }
      field_start = (curr == ',' || curr == '\n');
    }
  }
  if (in_quote) {
    return Invalid{"Ambiguity found with quote due to double quotes"};
  }
  reader.clear();
  reader.seekg(0);
  return Valid{};
}
