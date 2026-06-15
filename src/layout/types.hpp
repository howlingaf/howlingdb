#pragma once
#include <cstdint>
#include <string>

constexpr uint8_t PAGE_LIMIT = 32;

constexpr uint16_t PAGE_SIZE = 4096;

using header_t = uint32_t;
using entries_t = uint16_t;
using free_space_t = uint16_t;

using ident_t = uint8_t;
using slot_t = uint32_t;
using offset_t = uint16_t;
using length_t = uint16_t;

constexpr entries_t ENTRIES_OFFSET = 0;
constexpr offset_t FREE_SPACE_OFFSET = 2;

enum ColumnType : std::uint8_t { INT, FLOAT, VARCHAR };

struct Column {
  std::string name;
  ColumnType type;
};
