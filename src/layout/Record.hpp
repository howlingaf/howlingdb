#pragma once
#include <cstdint>
#include <memory>

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <printf.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "../catalog/Schema.hpp"
#include "types.hpp"
#include "utils.hpp"

constexpr uint16_t ROUND_UP = 7;

// add -Wall -Wextra -Wconversion -Werror flags to ur compiler
//  next, grind on templates and look into <type_traits> header, it has lots of
//  good helpers

// ============================================================================
// Variable-length record layout
//   instructor(ID, name, dept_name, salary)
//     - ID, name, dept_name : varchar -> stored as (offset, length) pairs
//     - salary              : fixed-length (lives in the fixed portion)
//
// 0       4        8        12              20 21      26           36 45
// +-------+--------+--------+---------------+--+-------+------------+------------+
// | 21, 5 | 26, 10 | 36, 10 |     65000     |  | 10101 | Srinivasan |
// Comp.Sci
// +-------+--------+--------+---------------+--+-------+------------+------------+
//                                            ^
//                                            null bitmask (1 byte) = 0000
//
//   (21, 5)  -> ID        @ byte 21, len 5   -> "10101"
//   (26, 10) -> name      @ byte 26, len 10  -> "Srinivasan"
//   (36, 10) -> dept_name @ byte 36, len 10  -> "Comp. Sci."
//   65000    -> salary    (fixed-length, in the fixed part of the record)
//
//   Fixed part = the (offset,length) pairs + fixed-length attrs + null
//   bitmask. Variable-length buf is appended after, referenced by the
//   offset pairs. Null bitmask: bit i set => attribute i is NULL (0000 here
//   => none null).
// ============================================================================

struct Record {
  std::unique_ptr<uint8_t[]> buf;
  const Schema &schema;
  uint16_t size{};

  void print() {
    for (size_t ci = 0; ci < schema.columns.size(); ci++) {
      const uint16_t pos = static_cast<uint16_t>(ci * sizeof(slot_t));
      const Slot slot = Slot(pos);
      switch (schema.columns[ci].type) {
      case INT: {
        const int val = read<int>(buf.get(), pos);
        std::printf("Column {%zu}, Value: {%d} \n", ci, val);
        break;
      }
      case FLOAT: {
        const float val = read<float>(buf.get(), pos);
        std::printf("Column {%zu}, Value: {%f} \n", ci, val);
        break;
      }
      case VARCHAR: {
        const uint16_t off = read<uint16_t>(buf.get(), slot.offset_pos);
        const uint16_t len = read<uint16_t>(buf.get(), slot.length_pos);
        const std::string val = read(buf.get(), off, len);
        std::printf("Column {%zu}, Value: {%s} \n", ci, val.c_str());
        break;
      }
      default:
        break;
      };
    }
  }

  void update(const Column &col, std::string val) {
    uint16_t ci = col_pos(col);

    const uint16_t bitmask = bitmask_offset();
    auto ith = (ROUND_UP - (ci % CHAR_BIT));

    const Slot slot = Slot(static_cast<uint16_t>(ci * SLOT_SIZE));

    uint16_t curr_offset = read<uint16_t>(buf.get(), slot.offset_pos);
    const uint16_t curr_length = read<uint16_t>(buf.get(), slot.length_pos);
    const uint16_t new_length = static_cast<uint16_t>(val.size());

    if (new_length <= curr_length) {

      write(buf.get(), curr_offset, val.data(), new_length);
      write(buf.get(), slot.length_pos, new_length);

      if (0 < new_length) {
        buf[bitmask + ci / CHAR_BIT] |= static_cast<uint8_t>(1 << ith);
      } else
        buf[bitmask + ci / CHAR_BIT] &= static_cast<uint8_t>(~(1 << ith));

      curr_offset += new_length;

      for (ci++; ci < schema.columns.size(); ci++) {
        if (schema.columns[ci].type == INT)
          continue;
        if (schema.columns[ci].type == FLOAT)
          continue;

        const uint16_t pos = static_cast<uint16_t>(ci * SLOT_SIZE);
        const Slot slot = Slot(pos);
        const uint16_t off = read<uint16_t>(buf.get(), slot.offset_pos);
        const uint16_t len = read<uint16_t>(buf.get(), slot.length_pos);

        write(buf.get(), curr_offset, &buf[off], len);
        write(buf.get(), pos, curr_offset);
        write(buf.get(), pos + OFFSET_SIZE, len);

        curr_offset += len;
      }
    }

    if (curr_length < new_length) {
      const uint16_t extra = static_cast<uint16_t>(new_length - curr_length);
      const uint16_t next = curr_offset + curr_length;
      auto entry = reinterpret_cast<unsigned char *>(val.data()); // TODO: ?
      const uint16_t end = buf_end();

      buf[bitmask + ci / CHAR_BIT] |= static_cast<uint8_t>(1 << ith);

      write(buf.get(), next + extra, next, end - next);
      write(buf.get(), curr_offset, entry, new_length);
      write(buf.get(), slot.length_pos, new_length);

      for (ci++; ci < schema.columns.size(); ci++) {
        if (schema.columns[ci].type == INT)
          continue;
        if (schema.columns[ci].type == FLOAT)
          continue;

        const uint16_t pos = static_cast<uint16_t>(ci * SLOT_SIZE);
        const Slot slot = Slot(pos);
        const uint16_t off = read<uint16_t>(buf.get(), slot.offset_pos);
        write(buf.get(), slot.offset_pos, off + extra);
      }
    }
    size = new_length;
  };

  template <typename T> void update(const Column &col, std::optional<T> opt) {

    const size_t ci = col_pos(col);
    const uint16_t bitmask = bitmask_offset();
    auto ith = (ROUND_UP - (ci % CHAR_BIT));

    if (opt.has_value()) {
      const uint16_t slot = ci * sizeof(slot_t);
      buf[bitmask + ci / CHAR_BIT] |= static_cast<uint8_t>(1 << ith);
      write(buf.get(), slot, opt.value());
    } else
      buf[bitmask + ci / CHAR_BIT] &= static_cast<uint8_t>(~(1 << ith));
  }

  uint16_t buf_end() {
    for (size_t ci = schema.columns.size(); ci-- > 0;) {
      if (schema.columns[ci].type == VARCHAR) {
        const uint16_t pos = static_cast<uint16_t>(ci);
        Slot slot = Slot(pos);
        return slot.offset_pos + slot.length_pos;
      };
    }
    return static_cast<uint16_t>(schema.columns.size() * SLOT_SIZE);
  }

  uint16_t bitmask_offset() {
    uint16_t off = 0;
    for (auto col : schema.columns) {
      if (col.type == INT)
        off += static_cast<uint16_t>(sizeof(int));
      if (col.type == FLOAT)
        off += static_cast<uint16_t>(sizeof(float));
      if (col.type == VARCHAR)
        off += static_cast<uint16_t>(SLOT_SIZE);
    }
    return off;
  }

  uint16_t col_pos(const Column &col) {
    for (size_t i = 0; i < schema.columns.size(); i++) {
      if (schema.columns[i].name == col.name &&
          schema.columns[i].type == col.type) {
        return static_cast<uint16_t>(i);
      }
    }
    throw std::runtime_error{"col not found"};
  }

  uint16_t var_length(const Column &col) {
    for (uint16_t ci = 0; ci < schema.columns.size(); ci++) {
      if (schema.columns[ci].name == col.name &&
          schema.columns[ci].type == col.type) {
        Slot slot = Slot(static_cast<uint16_t>(ci * SLOT_SIZE));
        return read<uint16_t>(buf.get(), slot.length_pos);
      }
    }
    throw std::runtime_error{"col not found"};
  }
};

inline Record create_record(const std::vector<std::string> &fields,
                            const Schema &schema) {

  uint16_t capacity = 0;
  uint16_t fixed_capacity = 0;
  uint16_t var_capacity = 0;

  for (size_t ci = 0; ci < schema.columns.size(); ++ci) {

    switch (schema.columns[ci].type) {
    case INT:
      capacity += static_cast<uint16_t>(sizeof(int));
      fixed_capacity += static_cast<uint16_t>(sizeof(int));
      break;
    case FLOAT:
      capacity += static_cast<uint16_t>(sizeof(float));
      fixed_capacity += static_cast<uint16_t>(sizeof(float));
      break;
    case VARCHAR: {
      capacity += static_cast<uint16_t>(SLOT_SIZE);
      const uint16_t len = static_cast<uint16_t>(fields[ci].size());
      capacity += len;
      var_capacity += len;
      break;
    }
    default:
      throw std::runtime_error("");
    }
  }

  const uint16_t cols_len = static_cast<uint16_t>(schema.columns.size());
  const uint16_t bytes =
      static_cast<uint16_t>((cols_len + ROUND_UP) / CHAR_BIT);
  capacity += static_cast<uint16_t>(sizeof(bytes));
  const uint16_t bitmask_offset = fixed_capacity;

  auto buf = std::make_unique<uint8_t[]>(capacity);
  std::fill(buf.get(), buf.get() + capacity, 1);
  std::memset(buf.get(), 0xFF, capacity);

  uint16_t fixed_offset = 0;
  uint16_t var_offset = capacity - var_capacity;
  for (size_t ci = 0; ci < fields.size(); ++ci) {

    const uint8_t mask =
        static_cast<uint8_t>(1 << (ROUND_UP - (ci % CHAR_BIT)));
    switch (schema.columns[ci].type) {
    case INT: {
      if (!fields[ci].empty()) {
        const int val_as_int = std::stoi(fields[ci]);
        std::printf("{%d}, fixed_offset={%d}\n", val_as_int, fixed_offset);
        write(buf.get(), fixed_offset, val_as_int);
        buf[bitmask_offset + (ci / CHAR_BIT)] |= mask;
      } else {
        write(buf.get(), fixed_offset, std::nullopt);
      }
      fixed_offset += static_cast<uint16_t>(sizeof(int));
      break;
    }

    case FLOAT: {
      if (!fields[ci].empty()) {
        const float val_as_float = std::stof(fields[ci]);
        std::printf("Float {%f}, fixed_offset={%d}\n", val_as_float,
                    fixed_offset);
        write(buf.get(), fixed_offset, val_as_float);
        buf[bitmask_offset + (ci / CHAR_BIT)] |= mask;
      } else {
        write(buf.get(), fixed_offset, std::nullopt);
      }
      fixed_offset += static_cast<uint16_t>(sizeof(float));
      break;
    }

    case VARCHAR: {
      const uint16_t len = static_cast<uint16_t>(fields[ci].size());
      write(buf.get(), fixed_offset, var_offset);
      const uint16_t len_offset =
          fixed_offset + static_cast<uint16_t>(sizeof(uint16_t));
      write(buf.get(), len_offset, len);
      fixed_offset += static_cast<uint16_t>(sizeof(slot_t));
      if (len == 0)
        buf[fixed_offset + (ci / CHAR_BIT)] &= ~mask;
      else {
        write(buf.get(), var_offset, fields[ci].data(), len);
        buf[bitmask_offset + (ci / CHAR_BIT)] |= mask;
      }
      var_offset += len;
      break;
    }
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }
  std::cout << '\n';
  return Record{std::move(buf), schema, capacity};
}
