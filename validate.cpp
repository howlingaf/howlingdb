#include "types.hpp"
#include <fstream>

ValidatorResult validate(std::ifstream &reader) {

  if (reader.peek() == EOF) {
    return Invalid{"File is empty."};
  }

  reader.clear();
  reader.seekg(0);
  bool in_quote = false;
  bool field_start = true;
  int row = 0;
  int col = 0;
  while (!reader.eof()) {
    char curr = reader.get();
    unsigned char currd = (unsigned char)curr;
    if (currd < 32 && currd != 9 && currd != 10 && currd != 13) {
      return Invalid{"Invalid characters found (likely not text file).\n", row,
                     col};
    }

    if (in_quote) {
      char next = reader.peek();
      if (curr == '"') {
        if (next == '"') {
          reader.ignore();

        } else if (next == '\n' || next == ',' || next == '\r') {
          in_quote = false;
        } else {
          return Invalid{"Ambiguity found within quote due to double quotes\n",
                         row, col};
        }
      }
    } else {
      if (curr == '"' && field_start) {
        in_quote = true;
      }
      if (curr == ',') {
        field_start = true;
      } else if (curr == '\n') {
        field_start = (curr == ',' || curr == '\n' || curr == '\r');
        row++;
        col = 0;
      }
    }
    col++;
  }
  if (in_quote) {
    return Invalid{"Ambiguity found within quote due to double quotes\n", row,
                   col};
  }
  reader.clear();
  reader.seekg(0);
  return Valid{};
}
