#pragma once

#include <memory>
#include <string>
#include <vector>
enum ColumnType { INT, FLOAT, VARCHAR };

struct Column {
  std::string name;
  ColumnType type;
};

struct Record {
  std::unique_ptr<uint8_t[]> data;
  uint16_t bytes;
  uint16_t id;
};

struct Schema {
  std::vector<Column> columns;
};
