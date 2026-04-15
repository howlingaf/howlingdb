#pragma once

#include "types.hpp"
#include <string>
#include <vector>

Schema create_schema(std::vector<std::string> &keys,
                     std::vector<std::string> &first_row) {

  Schema schema;
  for (auto i = 0; i < keys.size(); i++) {
    try {
      std::stoi(first_row[i]);
      Column column;
      column.type = INT;
      column.name = first_row[i];
      schema.columns.push_back(column);
    } catch (...) {
      try {
        std::stof(first_row[i]);
        Column column;
        column.type = FLOAT;
        column.name = first_row[i];
        schema.columns.push_back(column);
      } catch (...) {
        Column column;
        column.type = VARCHAR;
        column.name = first_row[i];
        schema.columns.push_back(column);
      }
    }
  }
  return schema;
}
