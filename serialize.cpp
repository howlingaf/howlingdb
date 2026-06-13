#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/types/layout.hpp"
#include "src/util/bytes.hpp"
#include "types.hpp"

Record serialize(const std::vector<std::string> &fields, const Schema &schema) {

  uint16_t capacity = 0;
  uint16_t fixed_capacity = 0;
  uint16_t var_capacity = 0;

  for (size_t i = 0; i < schema.columns.size(); ++i) {

    switch (schema.columns[i].type) {
    case INT:
      capacity += sizeof(int);
      fixed_capacity += sizeof(int);
      break;
    case FLOAT:
      capacity += sizeof(float);
      fixed_capacity += sizeof(float);
      break;
    case VARCHAR:
      capacity += fields[i].size();
      var_capacity += fields[i].size();
      break;
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }

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
  //                                            null bitmap (1 byte) = 0000
  //
  //   (21, 5)  -> ID        @ byte 21, len 5   -> "10101"
  //   (26, 10) -> name      @ byte 26, len 10  -> "Srinivasan"
  //   (36, 10) -> dept_name @ byte 36, len 10  -> "Comp. Sci."
  //   65000    -> salary    (fixed-length, in the fixed part of the record)
  //
  //   Fixed part = the (offset,length) pairs + fixed-length attrs + null
  //   bitmap. Variable-length data is appended after, referenced by the offset
  //   pairs. Null bitmap: bit i set => attribute i is NULL (0000 here => none
  //   null).
  // ============================================================================

  const uint8_t ROUND_UP = 7;
  const uint16_t bytes = (schema.columns.size() + ROUND_UP) / CHAR_BIT;

  capacity += sizeof(bytes);
  const uint8_t bitmap = fixed_capacity;

  auto buf = std::make_unique<uint8_t[]>(capacity);

  offset_t fixed_offset = 0;
  offset_t var_offset = capacity - var_capacity;
  write(buf.get(), bitmap, bytes);
  for (size_t i = 0; i < fields.size(); ++i) {
    if (!fields[i].empty()) {
      buf[fixed_offset + (i / CHAR_BIT)] |= 1 << (ROUND_UP - (i % CHAR_BIT));
    }

    std::cout << fields[i] << " ";
    switch (schema.columns[i].type) {
    case INT: {
      const int val_as_int = std::stoi(fields[i]);
      write(buf.get(), fixed_offset, val_as_int);
      fixed_offset += sizeof(int);
      break;
    }
    case FLOAT: {
      const float val_as_float = std::stof(fields[i]);
      write(buf.get(), fixed_offset, val_as_float);
      fixed_offset += sizeof(float);
      break;
    }
    case VARCHAR: {
      const length_t length = fields[i].size();
      write(buf.get(), fixed_offset, var_offset);
      write(buf.get(), fixed_offset + sizeof(offset_t), length);
      fixed_offset += (sizeof(offset_t) + sizeof(length_t));
      write(buf.get(), var_offset, fields[i].data(), sizeof(fields[i].data()));
      var_offset += length;
      break;
    }
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }

  std::cout << '\n';
  return Record{.data = std::move(buf), .size = capacity};
}
