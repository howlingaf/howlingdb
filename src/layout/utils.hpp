#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

inline void write_null(uint8_t *data, uint16_t offset) {
  std::nullopt_t null = std::nullopt;
  std::memcpy(&data[offset], &null, (uint16_t)1);
}

template <typename T> void write(uint8_t *data, uint16_t offset, const T &val) {
  std::memcpy(&data[offset], &val, sizeof(T));
};

template <typename T>
void write(uint8_t *data, uint16_t offset, const T *val, uint16_t length) {
  std::memcpy(&data[offset], val, length);
};
inline void write(uint8_t *data, uint16_t offset_1, uint16_t offset_2,
                  uint16_t length) {
  std::memmove(&data[offset_1], &data[offset_2], length);
};

inline std::string read(uint8_t *data, uint16_t offset, uint16_t length) {
  std::string val(reinterpret_cast<const char *>(&data[offset]), length);
  return val;
};

template <typename T>
T read(const uint8_t *data, const uint16_t offset, const uint16_t length) {
  T val{};
  std::memcpy(&val, &data[offset], length);
  return val;
};

template <typename T> T read(const uint8_t *data, const uint16_t offset) {
  T val{};
  std::memcpy(&val, &data[offset], sizeof(T));
  return val;
};
