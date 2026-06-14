#pragma once
#include <cstdint>
#include <cstring>

#include "types.hpp"

template <typename T> void write(uint8_t *data, offset_t offset, const T &val) {
  std::memcpy(&data[offset], &val, sizeof(val));
};

template <typename T>
void write(uint8_t *data, offset_t offset, const T *val, length_t length) {
  std::memcpy(&data[offset], val, length);
};

// handles overlap when moving from one offset to another
void write(uint8_t *data, offset_t offset_1, offset_t offset_2,
           length_t length) {
  std::memmove(&data[offset_1], &data[offset_2], length);
};

template <typename T> T read(uint8_t *data, offset_t offset, length_t length) {
  T val{};
  std::memcpy(&val, &data[offset], length);
  return val;
};

template <typename T> T read(uint8_t *data, offset_t offset) {
  T val{};
  std::memcpy(&val, &data[offset], sizeof(T));
  return val;
};
