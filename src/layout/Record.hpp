#pragma once
#include <cstdint>
#include <memory>

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <string>
#include <optional>
#include <printf.h>
#include <algorithm>

#include "../catalog/Schema.hpp"
#include "types.hpp"
#include "utils.hpp"

constexpr uint16_t ROUND_UP = 7;

//add -Wall -Wextra -Wconversion -Werror flags to ur compiler
// next, grind on templates and look into <type_traits> header, it has lots of good helpers

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
//   bitmask. Variable-length data is appended after, referenced by the
//   offset pairs. Null bitmask: bit i set => attribute i is NULL (0000 here
//   => none null).
// ============================================================================

struct Record {
  std::unique_ptr<uint8_t[]> data;
  const Schema& schema;
  size_t size{};

  void print(){
    for (size_t ci = 0; ci < schema.columns.size(); ci++){
      const offset_t pos = static_cast<offset_t>(ci * sizeof(slot_t));
      const offset_t off = read<offset_t>(data.get(), pos);
      const length_t off_sz = static_cast<length_t>(sizeof(offset_t));
      switch (schema.columns[ci].type){
        case INT: {
          const int val = read<int>(data.get(), pos);
          std::printf("Column {%zu}, Value: {%d} \n",ci, val);
          break;
        }
        case FLOAT: {
          const float val = read<float>(data.get(), pos);
          std::printf("Column {%zu}, Value: {%f} \n", ci, val);
          break;
        }
        case VARCHAR: {
          const offset_t lpos = static_cast<offset_t>(pos + off_sz);
          const length_t len = read<length_t>(data.get(), lpos);
          const std::string val = read(data.get(), off, len);
          std::printf("Column {%zu}, Value: {%s} \n", ci, val.c_str());
          break;
        }
        default:
          break;
      };
    }
  }

  void update(const Column& col, std::string val){
    offset_t ci = col_pos(col);
    
    const offset_t bitmask = bitmask_offset();
    auto ith = (ROUND_UP - (ci % CHAR_BIT));

    const offset_t slot = static_cast<offset_t>(ci * sizeof(slot_t));

    const offset_t offset = read<offset_t>(data.get(), slot);
    const length_t length = read<offset_t>(data.get(), slot + sizeof(offset_t));

    if (val.size() <= length){
      offset_t insert = offset;
      write(data.get(), insert, val.data(), (length_t)val.size());
      write(data.get(), slot + sizeof(offset_t),(length_t)val.size());
      if (0 < val.size()){
        data[bitmask + ci/CHAR_BIT] |= static_cast<uint8_t>(1 << ith);
      } else data[bitmask + ci/CHAR_BIT] &= static_cast<uint8_t>(~(1 << ith));

      insert+=(length_t)val.size();
      for (ci++; ci < schema.columns.size(); ci++){
        if (schema.columns[ci].type == INT) continue;
        if (schema.columns[ci].type == FLOAT) continue;

        const offset_t slot = static_cast<offset_t>(ci * sizeof(slot_t));
        const offset_t off = read<offset_t>(data.get(), slot);
        const length_t len = read<length_t>(data.get(), slot + sizeof(offset_t));
        auto entry = &data[off];
        write(data.get(), insert, entry, len);
        write(data.get(), slot, insert);
        write(data.get(), slot + sizeof(offset_t), len);
        insert += len;
      }
    }


    if (length < val.size()){
      const length_t extra = static_cast<length_t>(val.size() - length);
      const offset_t off = offset;
      const length_t len = static_cast<length_t>(val.size());
      const offset_t next = offset + length;
      auto entry = reinterpret_cast<unsigned char*>(val.data());
      const offset_t end = data_end();

      data[bitmask + ci/CHAR_BIT] |= static_cast<uint8_t>( 1 << ith);

      write(data.get(), next + extra, next, end - next);
      write(data.get(), off, entry, len);
      write(data.get(), slot + sizeof(offset_t), len);

      for (ci++; ci < schema.columns.size(); ci++){
        if (schema.columns[ci].type == INT) continue;
        if (schema.columns[ci].type == FLOAT) continue;

        const offset_t slot = static_cast<offset_t>(ci * sizeof(slot_t));
        const offset_t noff = read<offset_t>(data.get(), slot);
        write(data.get(), slot, noff + extra);
      }
    }
  };

  template <typename T> 
  void update(const Column& col, std::optional<T> opt){

    const size_t ci = col_pos(col);
    const offset_t bitmask = bitmask_offset();
    auto ith = (ROUND_UP - (ci % CHAR_BIT));

    if (opt.has_value()){
      const offset_t slot = ci * sizeof(slot_t);
      data[bitmask + ci/CHAR_BIT] |= static_cast<uint8_t>( 1 << ith);
      write(data.get(), slot, opt.value());
    } else data[bitmask + ci/CHAR_BIT] &= static_cast<uint8_t>(~(1 << ith));

  }
  
  offset_t data_end (){
    for (size_t i = schema.columns.size(); i-- > 0; ){
      if (schema.columns[i].type == VARCHAR){
        const uint16_t pos = static_cast<offset_t>(i);
        const offset_t off = read<offset_t>(data.get(), pos);
        const length_t len = read<length_t>(data.get(), pos + sizeof(offset_t));
        return off + len;
      }
    }
    return static_cast<offset_t>(schema.columns.size() * sizeof(slot_t));
  }

  offset_t bitmask_offset(){
    offset_t off{};
    for (auto col : schema.columns){
      if (col.type == INT) off+=static_cast<offset_t>(sizeof(int));
      if (col.type == FLOAT) off+=static_cast<offset_t>(sizeof(float));
      if (col.type == VARCHAR) off+=static_cast<offset_t>(sizeof(slot_t));
    }
    return off;
  }

  offset_t col_pos(const Column& col){
    for (size_t i = 0; i < schema.columns.size(); i++){
      if (schema.columns[i].name == col.name && schema.columns[i].type == col.type){
        return static_cast<offset_t>(i);
      }
    }
    throw std::runtime_error{"col not found"};
  }

  length_t var_length(const Column& col){
    for (offset_t i = 0; i < schema.columns.size(); i++){
      if (schema.columns[i].name == col.name && schema.columns[i].type == col.type){
        const offset_t opos = static_cast<offset_t>(i * sizeof(slot_t));
        const offset_t lpos = static_cast<offset_t>(opos + sizeof(offset_t));
        const length_t len = read<length_t>(data.get(), lpos);
        return len;
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

  for (size_t i = 0; i < schema.columns.size(); ++i) {

    switch (schema.columns[i].type) {
    case INT:
      capacity += static_cast<uint16_t>(sizeof(int));
      fixed_capacity += static_cast<uint16_t>(sizeof(int));
      break;
    case FLOAT:
      capacity += static_cast<uint16_t>(sizeof(float));
      fixed_capacity += static_cast<uint16_t>(sizeof(float));
      break;
    case VARCHAR: {
      capacity += static_cast<uint16_t>(sizeof(slot_t));
      const uint16_t len = static_cast<uint16_t>(fields[i].size());
      capacity += len;
      var_capacity += len;
      break;
    }
    default:
      throw std::runtime_error("");
    }
  }

  const uint16_t cols_len = static_cast<uint16_t>(schema.columns.size());
  const uint16_t bytes = static_cast<uint16_t>((cols_len + ROUND_UP) / CHAR_BIT);
  capacity += static_cast<uint16_t>(sizeof(bytes));
  const uint16_t bitmask_offset = fixed_capacity;
  
  auto buf = std::make_unique<uint8_t[]>(capacity);
  std::fill(buf.get(),buf.get() + capacity, 1 );
  std::memset(buf.get(), 0xFF, capacity);

  offset_t fixed_offset = 0;
  offset_t var_offset = capacity - var_capacity;
  for (size_t i = 0; i < fields.size(); ++i) {

    const uint8_t mask = static_cast<uint8_t>(1 << (ROUND_UP - (i % CHAR_BIT)));
    switch (schema.columns[i].type) {
    case INT: {
      if (!fields[i].empty()){
        const int val_as_int = std::stoi(fields[i]);
        std::printf("{%d}, fixed_offset={%d}\n", val_as_int, fixed_offset);
        write(buf.get(), fixed_offset, val_as_int);
        buf[bitmask_offset + (i / CHAR_BIT)] |= mask;
      } else {
        write(buf.get(), fixed_offset, std::nullopt);
      }
      fixed_offset += static_cast<offset_t>(sizeof(int));
      break;
    }

    case FLOAT: {
      if (!fields[i].empty()){
        const float val_as_float = std::stof(fields[i]);
        std::printf("Float {%f}, fixed_offset={%d}\n", val_as_float, fixed_offset);
        write(buf.get(), fixed_offset, val_as_float);
        buf[bitmask_offset + (i / CHAR_BIT)] |= mask;
      } else {
        write(buf.get(), fixed_offset, std::nullopt);
      }
      fixed_offset += static_cast<offset_t>(sizeof(float));
      break;
    }

    case VARCHAR: {
      const length_t len = static_cast<length_t>(fields[i].size());
      write(buf.get(), fixed_offset, var_offset);
      const offset_t len_offset = fixed_offset + static_cast<offset_t>(sizeof(offset_t));
      write(buf.get(), len_offset , len);
      fixed_offset += static_cast<offset_t>(sizeof(slot_t));
      if (len == 0) buf[fixed_offset + (i / CHAR_BIT)] &= ~mask;
      else {
        write(buf.get(), var_offset, fields[i].data(), len);
        buf[bitmask_offset + (i / CHAR_BIT)] |= mask;
      } 
      var_offset += len;
      break;
    }
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }
  std::cout << '\n';
  return Record{std::move(buf), schema, capacity };
}
