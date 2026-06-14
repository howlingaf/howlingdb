#include <fstream>

enum Result : uint8_t {
  SUCCESS,
  PARTIAL,
  FAIL,
};

const int INVALID_CHAR_UPPER_BOUND = 32;

Result validate(std::ifstream &reader) {

  if (reader.peek() == EOF) {
    return FAIL;
  }

  reader.clear();
  reader.seekg(0);
  bool in_quote = false;
  bool field_start = true;
  int row = 0;
  int col = 0;
  while (!reader.eof()) {
    const int curr = reader.get();
    const int currd = (unsigned char)curr;
    if (currd < INVALID_CHAR_UPPER_BOUND) {
      return FAIL;
    }

    if (in_quote) {
      const int next = reader.peek();
      if (curr == '"') {
        if (next == '"') {
          reader.ignore();

        } else if (next == '\n' || next == ',' || next == '\r') {
          in_quote = false;
        } else {
          return FAIL;
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
    return FAIL;
  }
  reader.clear();
  reader.seekg(0);
  return SUCCESS;
}
