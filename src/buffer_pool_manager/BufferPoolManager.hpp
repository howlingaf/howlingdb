#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "../catalog/Schema.hpp"
#include "../disk_manager/DiskManager.hpp"

struct LRUCache {

  static constexpr uint8_t POOL_LIMIT = 32;

  DiskManager disk_manager;
  std::vector<Page> pages;
  // TODO: pin/unpin for concurrency

  Page &top() { return pages.back(); }

  std::optional<Page> fetch(uint8_t page_id) {
    if (is_present(page_id)) {
      return evict(page_id);
    }
    std::optional<Page> page = disk_manager.retrieve(page_id);
    return page;
  }
  bool is_present(uint8_t page_id) {
    for (const auto page : pages) {
      if (page.id == page_id)
        return true;
    }
    return false;
  }
  // TODO: erase does linear lookup??
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
    const Page page = *pages.begin();
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

struct BufferPoolManager {
  LRUCache pool;

  // TODO: checkout/release on pool
  Page curr_page;
  Schema schema;

  BufferPoolManager();

  // TODO: needs pool and disk check
  int insert(Record record) {

    if (curr_page.free_space < record.size) {
      // look at pool
      // look at disk
      // if neither, create
    }
    curr_page.insert(record);
    return 0;
  }

  // TODO: needs pool and disk check
  std::optional<Page> fetch(ident_t page_id) {
    if (page_id == curr_page.id)
      return curr_page;
    std::optional<Page> page = pool.fetch(page_id);
    if (!page.has_value()) {
      return std::nullopt;
    }
    pool.add(curr_page);
    return page;
  }

  // TODO: needs pool and disk check
  int remove(ident_t page_id, ident_t row_id) {
    if (curr_page.id == page_id) {
      curr_page.remove(row_id);
      return 0;
    }
    return 1;
  }

  template <typename T> int update(ident_t page_id, ident_t row_id, T val) {

    if (curr_page.id == page_id) {
      curr_page.update(schema, page_id, row_id, val);
      return 0;
    }

    // check pool, disk
    // if (pool.is_present(page_id)) {
    //   pool.update_pos(page_id);
    //   curr_page = pool.top();
    // }
    //

    return 0;
  }

  int ingest(std::ifstream &reader) {
    std::cout << "Initiating ingestion" << '\n';
    bool IN_VALUE = false;
    std::vector<std::string> headers;
    std::vector<std::string> row;
    uint64_t count = -1;
    std::stringstream value;
    bool field_start = true;
    while (!reader.eof()) {
      const uint8_t curr = reader.get();
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
        const uint8_t next = reader.peek();
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
          insert(std::move(record));
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
