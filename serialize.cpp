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
  uint16_t bitmap_bytes = (schema.columns.size() + 7) / 8;
  std::vector<uint16_t> bitmap(bitmap_bytes, 0);
  uint16_t total_bytes = 0;
  uint8_t fixed_pos = 0;
  uint8_t var_pos = bitmap_bytes;

  for (size_t i = 0; i < schema.columns.size(); ++i) {
    switch (schema.columns[i].type) {
    case INT:
      total_bytes += sizeof(int);
      break;
    case FLOAT:
      total_bytes += sizeof(float);
      break;
    case VARCHAR:
      total_bytes += fields[i].size();
      break;
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }
  auto buf = std::make_unique<uint8_t[]>(total_bytes);
  uint8_t *write_ptr = buf.get();
  for (uint8_t i = 0; i < fields.size(); ++i) {

    if (fields[i].empty()) {
      bitmap[i / 8] |= 1 << (7 - (i % 8));
      continue;
    }
    std::cout << fields[i] << " ";
    ;
    switch (schema.columns[i].type) {
    case INT: {
      int int_val = std::stoi(fields[i]);
      std::memcpy(&buf[fixed_pos], &int_val, 4);
      fixed_pos += 4;
      break;
    }
    case FLOAT: {
      double fl_val = std::stof(fields[i]);
      std::memcpy(&buf[fixed_pos], &fl_val, 8);
      fixed_pos += 8;
      break;
    }
    case VARCHAR: {
      uint16_t offset = var_pos;
      uint16_t length = fields[i].size();
      std::memcpy(&buf[fixed_pos], &offset, 2);
      fixed_pos += 2;
      std::memcpy(&buf[fixed_pos], &length, 2);
      fixed_pos += 2;
      std::memcpy(&buf[var_pos], fields[i].data(), length);
      var_pos += length;
      break;
    }
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }
  std::cout << std::endl;
  return Record{std::move(buf), total_bytes};
}
