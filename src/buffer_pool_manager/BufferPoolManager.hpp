#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>

#include "../catalog/Schema.hpp"
#include "../disk_manager/DiskManager.hpp"

struct LRUCache {

  static constexpr uint8_t POOL_LIMIT = 32;

  DiskManager disk_manager;
  std::vector<Page> pages;

  Page &top() { return pages.back(); }

  std::optional<Page> fetch(uint8_t page_id) {
    if (is_present(page_id)) {
      return evict(page_id);
    }
    std::optional<Page> page = disk_manager.retrieve(page_id);
    return page;
  }
  bool is_present(uint8_t page_id) {
    for (auto page : pages) {
      if (page.id == page_id)
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
  std::optional<Page> curr_page;
  // TODO: Pins

  int insert(const Schema &schema, Record record) {
    const uint16_t len = static_cast<uint16_t>(record.size);
    if (!curr_page.has_value()) {
      curr_page = Page(schema);
    }
    if (curr_page->free_space < len) {
      return 0;
      // look at pool
      // look at disk
      // if neither, create
    }
    curr_page->insert(record);
    curr_page->print();
    return 0;
  }

  // TODO: needs pool and disk check
  std::optional<Page> fetch(ident_t page_id) {
    if (page_id == curr_page->id)
      return curr_page;
    std::optional<Page> page = pool.fetch(page_id);
    if (!curr_page.has_value()) {
      return std::nullopt;
    }
    pool.add(curr_page.value());
    return page;
  }

  // TODO: needs pool and disk check
  int remove(ident_t page_id, ident_t row_id) {
    if (curr_page->id == page_id) {
      curr_page->remove(row_id);
      return 0;
    }
    return 1;
  }

  template <typename T>
  int update(const Schema &schema, const ident_t page_id, const ident_t row_id,
             const T val) {

    if (curr_page->id == page_id) {
      curr_page->update(schema, page_id, row_id, val);
      return 0;
    }

    // TODO: Pool
    // TODO: Disk

    return 0;
  }
};
