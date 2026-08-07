#pragma once
#include <cstdint>
#include <string>

constexpr uint8_t PAGE_LIMIT = 32;
constexpr uint16_t PAGE_SIZE = 4096;

constexpr uint16_t OFFSET_SIZE = 2;
constexpr uint16_t LENGTH_SIZE = 2;
constexpr uint16_t FIXED_DATA_SIZE = 4;
constexpr uint16_t SLOT_SIZE = static_cast<uint16_t>(OFFSET_SIZE + LENGTH_SIZE);

using ident_t = uint8_t;
using slot_t = uint32_t;

constexpr uint16_t ENTRIES_OFFSET = 0;
constexpr uint16_t FREE_SPACE_OFFSET = 2;
constexpr uint16_t HEADER_SIZE =
    static_cast<uint16_t>(sizeof(ENTRIES_OFFSET) + sizeof(FREE_SPACE_OFFSET));

enum ColumnType : std::uint8_t { INT, FLOAT, VARCHAR };

struct Slot {
  uint16_t offset_pos;
  uint16_t length_pos;

  const uint16_t size = SLOT_SIZE;
  Slot(const uint16_t pos) {
    offset_pos = pos;
    length_pos = pos + LENGTH_SIZE;
  };
};

struct Column {
  std::string name;
  ColumnType type;
};
