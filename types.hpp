#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

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

enum ColumnType : std::uint8_t { INT, FLOAT, VARCHAR };

struct Column {
  std::string name;
  uint8_t type;
};

struct Schema {
  std::vector<Column> columns;
  uint8_t id;
};

Schema create_schema(std::vector<std::string> &keys,
                     std::vector<std::string> &first_row);

struct Record {
  std::unique_ptr<uint8_t[]> data;
  uint16_t size;
};

Record serialize(const std::vector<std::string> &fields, const Schema &schema);
