#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <printf.h>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "Page.hpp"
#include "schema.hpp"
#include "serialize.hpp"

struct Valid {};

struct Invalid {
  std::string reason;
  int error_row;
  int error_col;
};

using ValidatorResult = std::variant<Valid, Invalid>;

ValidatorResult is_csv_valid(std::ifstream &reader) {

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

enum State {
  Success,
  PartialSuccess,
  Failure,
};

struct ParseResult {
  State state;
  std::vector<std::string> invalid_rows;
  int error_code; // need some mapping code->error message
};

ParseResult parse(std::ifstream &csv); // return Success or HasUnresolved

void print_row(const std::vector<std::string> &row) {
  for (size_t i = 0; i < row.size(); i++) {
    std::cout << i << row.at(i) + "|";
  }
  std::cout << std::endl;
};

ParseResult parse(std::ifstream &reader) {
  int success_count;
  int failure_count;
  int runtime;
  std::cout << "Initiating parsing" << std::endl;
  bool IN_VALUE = false;
  std::vector<std::string> headers;
  std::vector<std::string> row;
  uint64_t count = -1;
  Schema schema;
  std::stringstream value;
  Page page;
  while (!reader.eof()) {
    char curr = reader.get();
    if (curr == '"') {
      if (!IN_VALUE) {
        auto pos = reader.tellg();
        if (pos == 1) {
          printf("%d\n", (int)reader.tellg());
          IN_VALUE = true;
        } else {
          value << curr;
        }
        continue;
      }
      char next = reader.peek();
      if (next == '\n' || next == ',') {
        IN_VALUE = false;
      } else if (next == '"') {
        value << curr;
        reader.ignore();
      } else {
        return ParseResult{
            Failure,
            {},
            1,
        };
      }
    } else if (curr == ',' && !IN_VALUE) {
      row.push_back(value.str());
      value = std::stringstream();
    } else if (curr == '\n' && !IN_VALUE) {
      row.push_back(value.str());
      value = std::stringstream();
      if (count == -1) {
        headers = row;
      } else {
        if (!headers.empty()) {
          schema = create_schema(headers, row);
          headers.clear();
        }
        const Record record = serialize(row, schema);
        page.insert_record(record);
        std::cout << "inserted" << std::endl;
      }
      count++;
      row.clear();
    } else {
      value << curr;
    }
  }
  return ParseResult{Success, {}, -1};
}
