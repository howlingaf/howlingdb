#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stddef.h>
#include <stdexcept>

#include "DiskManager.hpp"
#include "types.hpp"

struct LRUCache {
  static constexpr uint8_t POOL_LIMIT = 10;
  DiskManager disk_manager;
  std::vector<Page> pages;

  Page &top() { return pages.back(); }

  bool is_present(uint8_t id) {
    for (auto i = pages.begin(); i != pages.end(); i++) {
      if (i->id == id)
        return true;
    }
    return false;
  }
  void update_pos(uint8_t page_id) {
    for (auto i = pages.begin(); i != pages.end(); i++) {
      if (i->id == page_id) {
        auto temp = i;
        pages.erase(i);
        add(*temp);
        return;
      }
    }
  }
  void add(Page page) {
    if (is_full()) {
      evict();
    }
    pages.push_back(page);
  }
  bool is_full() { return pages.size() == POOL_LIMIT; }

  void evict() {
    Page page = *pages.begin();
    pages.erase(pages.begin());
    disk_manager.write(page);
  }

  Page evict(uint8_t page_id) {
    for (auto i = pages.begin(); i != pages.end(); i++) {
      if (i->id == page_id) {
        Page page = *i;
        pages.erase(i);
        return page;
      }
    }
    throw std::runtime_error{"page not found"};
  }
};

struct BufferManager {

  LRUCache pool;
  Page curr_page;
  Schema schema;

  int insert(Schema schema, Record record) {
    if (curr_page.free_space() < record.size) {
      pool.add(curr_page);
      curr_page.data.fill(0);
      curr_page.id++;
    }
    curr_page.insert(std::move(record));
    return 0;
  }

  Page fetch(uint8_t page_id) {
    if (page_id == curr_page.id)
      return curr_page;
    pool.add(curr_page);

    if (pool.is_present(page_id)) {
      curr_page = pool.evict(page_id);
      return curr_page;
    }
    Page new_page = disk_manager.fetch(page_id);
  }

  int update_record(Page page);
  uint8_t read(uint32_t page_id, uint32_t rowid) {
    // TODO:
    return 0;
  }
  uint8_t remove(uint32_t page_id, uint32_t rowid) {
    // TODO:
    //  find record position
    //  update pointers
    return 0;
  }
  int update(uint8_t page_id, uint32_t row_id, Record record) {
    if (curr_page.id == page_id) {
      // TODO: get by row_id
    }

    if (pool.is_present(page_id)) {
      pool.update(page_id);
      curr_page = pool.top();
    }
    disk_manager.fetch(page_id);
    return 0;
  }

  int ingest(std::ifstream &reader) {
    int success_count;
    int failure_count;
    int runtime;
    std::cout << "Initiating ingestion" << std::endl;
    bool IN_VALUE = false;
    std::vector<std::string> headers;
    std::vector<std::string> row;
    uint64_t count = -1;
    Schema schema;
    std::stringstream value;
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
          Record record = serialize(row, schema);
          if (record.size <= curr_page.free_space()) {
          }
          insert(schema, std::move(record));
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
};
