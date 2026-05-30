#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "types.hpp"

Record serialize(const std::vector<std::string> &fields, const Schema &schema) {

  uint8_t capacity = 0;
  uint8_t fixed_capacity = 0;
  uint8_t var_capacity = 0;

  for (size_t i = 0; i < schema.columns.size(); ++i) {

    switch (schema.columns[i].type) {
    case INT:
      capacity += sizeof(int);
      fixed_capacity += sizeof(int);
      break;
    case FLOAT:
      capacity += sizeof(float);
      fixed_capacity += sizeof(int);
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

  uint8_t bytes = (schema.columns.size() + 7) / 8;

  capacity += sizeof(bytes);
  uint8_t bitmapp = fixed_capacity;

  auto buf = std::make_unique<uint8_t[]>(capacity);

  uint8_t fixedp = 0;
  uint8_t varp = capacity - var_capacity;
  uint8_t *bufp = buf.get();

  std::memcpy(&buf[bitmapp], &bytes, sizeof(bytes));
  for (uint8_t i = 0; i < fields.size(); ++i) {
    if (!fields[i].empty()) {
      buf[fixedp + (i / 8)] |= 1 << (7 - (i % 8));
    }

    std::cout << fields[i] << " ";
    switch (schema.columns[i].type) {
    case INT: {
      int int_val = std::stoi(fields[i]);
      std::memcpy(&buf[fixedp], &int_val, sizeof(int_val));
      fixedp += sizeof(int_val);
      break;
    }
    case FLOAT: {
      double fl_val = std::stof(fields[i]);
      std::memcpy(&buf[fixedp], &fl_val, sizeof(fl_val));
      fixedp += sizeof(fl_val);
      break;
    }
    case VARCHAR: {
      uint16_t offset = varp;
      uint16_t length = fields[i].size();
      std::memcpy(&buf[fixedp], &offset, sizeof(offset));
      fixedp += offset;
      std::memcpy(&buf[fixedp], &length, sizeof(length));
      fixedp += sizeof(length);
      std::memcpy(&buf[varp], fields[i].data(), length);
      varp += length;
      break;
    }
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }

  std::cout << std::endl;
  return Record{std::move(buf), capacity};
}
