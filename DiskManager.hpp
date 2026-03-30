#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <variant>
#include <vector>

static constexpr uint32_t PAGE_SIZE = 4096;

// 128b
struct PageHeader {
  uint32_t record_count;
  uint32_t free_space_size;
  uint32_t free_space_offset;
  uint32_t header_offset;
};
// 16b
struct PageSlot {
  uint32_t offset;
  uint32_t length;
};

// 192b
struct PageRecord {
  std::string county;
  uint64_t population;
  uint32_t median_age;
  uint32_t median_income;

  // move string instead of reference
  PageRecord(const std::string &c, uint64_t p, uint32_t a, uint32_t i) {
    county = c;
    population = p;
    median_age = a;
    median_income = i;
  }
};

using PageData = std::variant<std::monostate, PageHeader, PageSlot, PageRecord>;

struct Page {
  std::array<PageData, PAGE_SIZE> data;
  PageData &operator[](size_t index) { return data[index]; }
};

class DiskManager {
  std::vector<Page> pages;
  uint32_t current_page_index = 0;
  int write(std::string &county, uint64_t population, uint32_t median_age,
            uint32_t median_income) {
    PageRecord record =
        PageRecord{county, population, median_age, median_income};

    if (current_page_index + 1 < pages.size()) {
      PageHeader page_header = {.record_count = 0,
                                .free_space_size = PAGE_SIZE,
                                .free_space_offset = PAGE_SIZE,
                                .header_offset = 0};

      Page page;
      page.data[page_header.header_offset] = page_header;
      page_header.header_offset += sizeof(PageHeader);
      page_header.free_space_size -= sizeof(PageHeader);
    };
    Page current_page = pages[current_page_index];

    PageHeader &page_header = std::get<PageHeader>(current_page.data[0]);
    uint32_t header_offset = page_header.header_offset;

    PageSlot slot = {.offset = header_offset, .length = sizeof(PageSlot)};

    current_page[header_offset] = slot;
    page_header.header_offset += sizeof(PageSlot);
    page_header.free_space_size += sizeof(PageSlot);

    uint32_t free_space_offset = page_header.free_space_offset;
    current_page[free_space_offset - sizeof(record)] = record;
    free_space_offset -= sizeof(record);
    page_header.free_space_size += sizeof(record);

    // update record struct to eqaul the input data

    // second pass parse, do a disk manager write

    // guard against unavailable space
    // create new page if necessary

    // take a linear to at worst case shift slots to the left and shift records
    // to the right
    return 0;
  }
};
