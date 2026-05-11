#pragma once

#include <array>
#include <cstring>
#include <memory>
#include <stdint.h>
#include <string>
#include <variant>
#include <vector>

enum State {
  Success,
  PartialSuccess,
  Failure,
};

inline constexpr uint16_t PAGE_SIZE = 4096;
inline constexpr uint16_t ENTRIESp = 0;
inline constexpr uint16_t FREE_SPACE_ENDp = 2;
inline constexpr uint16_t SLOT_START = 4;

struct Valid {};

struct Invalid {
  std::string reason;
  int error_row;
  int error_col;
};

using ValidatorResult = std::variant<Valid, Invalid>;

ValidatorResult validate(std::ifstream &reader);

enum ColumnType { INT, FLOAT, VARCHAR };

struct Column {
  std::string name;
  ColumnType type;
};

struct Schema {
  std::vector<Column> columns;
};

Schema create_schema(std::vector<std::string> &keys,
                     std::vector<std::string> &first_row);

struct Record {
  std::unique_ptr<uint8_t[]> data;
  uint32_t bytes;
};

Record serialize(const std::vector<std::string> &fields, const Schema &schema);

struct Page {
  uint64_t id;
  std::array<uint8_t, PAGE_SIZE> data;
  Page(uint64_t id) {
    std::memcpy(&data, &ENTRIESp, sizeof(ENTRIESp));
    std::memcpy(&data[FREE_SPACE_ENDp], &PAGE_SIZE, sizeof(PAGE_SIZE));
    id = id;
  };
};
