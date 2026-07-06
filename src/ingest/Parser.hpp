#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "../catalog/Schema.hpp"
#include "../layout/Record.hpp"
#include "../buffer_pool_manager/BufferPoolManager.hpp"


inline int ingest(std::ifstream &reader, BufferPoolManager& bpm){
  std::cout << "Initiating ingestion" << '\n';
  bool IN_VALUE = false;
  Schema schema;
  
  std::vector<std::string> headers;
  std::vector<std::string> row;
  int64_t count = -1;
  std::stringstream value;
  bool field_start = true;

  while (!reader.eof()) {
    const uint8_t curr = static_cast<uint8_t>(reader.get());
    if (curr == '"') {
      if (!IN_VALUE) {
        auto pos = reader.tellg();
        if (field_start) {
          field_start = false;
          IN_VALUE = true;
        } else {
          value << curr;
        }
        continue;
      }
      const uint8_t next = static_cast<uint8_t>(reader.peek());
      if (next == '\n' || next == ',') {
        IN_VALUE = false;
      } else if (next == '"') {
        value << curr;
        reader.ignore();
      } else {
        return 1;
      }
    } else if (curr == ',' && !IN_VALUE) {
      row.push_back(value.str());
      value = std::stringstream();
    } else if (curr == '\n' && !IN_VALUE) {
      row.push_back(value.str());
      value = std::stringstream();
      if (count == -1) {
        headers = row;
      } else {
        if (!headers.empty()) {
          schema = create_schema(headers, row);
          headers.clear();
        }
        Record record = create_record(row, schema);
        bpm.insert(schema, std::move(record));
      }
      count++;
      row.clear();
      field_start = true;
    } else {
      value << curr;
    }
  }
  return 0;
};
