#include <cstdint>
#include <fstream>
#include <ostream>
#include <sstream>

#include "PageManager.hpp"
#include "types.hpp"

struct IngestResult {
  State state;
  std::vector<std::string> invalid_rows;
  int error_code; // need some mapping code->error message
};

struct Ingest {
  PageManager page_manager;
  Ingest(PageManager pm) : page_manager(pm) {};

  IngestResult parse(std::ifstream &reader) {
    int success_count;
    int failure_count;
    int runtime;
    std::cout << "Initiating parsing" << std::endl;
    bool IN_VALUE = false;
    std::vector<std::string> headers;
    std::vector<std::string> row;
    uint64_t count = -1;
    Schema schema;
    std::stringstream value;
    Page page(page_manager.page);
    bool field_start = true;
    while (!reader.eof()) {
      char curr = reader.get();
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
        char next = reader.peek();
        if (next == '\n' || next == ',') {
          IN_VALUE = false;
        } else if (next == '"') {
          value << curr;
          reader.ignore();
        } else {
          return IngestResult{
              Failure,
              {},
              1,
          };
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
          const Record record = serialize(row, schema);
          page_manager.insert(record);
        }
        count++;
        row.clear();
        field_start = true;
      } else {
        value << curr;
      }
    }
    return IngestResult{Success, {}, -1};
  }
};
