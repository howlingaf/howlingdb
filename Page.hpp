#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <variant>
#include <vector>

// 128b
struct Header {
  uint32_t record_count;
  uint32_t free_space_size;
  uint32_t free_space_offset;
  uint32_t block_header_offset;
};

// 4byte
struct Slot {
  uint64_t offset;
  uint64_t length;
};

struct Record {
  uint64_t *data;
  uint16_t size;
};

class Page {

  static constexpr uint32_t PAGE_SIZE = 4096;

  using PageData = std::variant<std::monostate, Header, Slot, Record>;

  std::array<PageData, PAGE_SIZE> buffer;
  PageData &operator[](size_t index) { return buffer[index]; }

  Page() {
    Header page_header = {.record_count = 0,
                          .free_space_size = PAGE_SIZE,
                          .free_space_offset = PAGE_SIZE,
                          .block_header_offset = 0};
    buffer[page_header.block_header_offset] = page_header;
    page_header.block_header_offset += sizeof(Header);
    page_header.free_space_size -= sizeof(Header);
  };

  int insert_record(const uint8_t *data) {

    Header &header = get_header();

    Slot slot = {.offset = header.block_header_offset, .length = sizeof(Slot)};

    buffer[header.block_header_offset] = slot;
    header.block_header_offset += slot.length;
    header.free_space_size -= slot.length;

    // TODO: unpack data into records, iterate
    buffer[header.free_space_offset - sizeof(record)] = record;
    header.free_space_offset -= sizeof(record);
    // page_header.free_space_size -= sizeof(record);

    return 0;
  }

  Header &get_header() { return std::get<Header>(buffer[0]); }
};

Record serialize(const std::vector<std::string> &row) {
  Record record = Record{};
  uint16_t size_of_items = 0;
  for (auto it = row.begin(); it != row.end(); ++it) {
    size_of_items += sizeof(*it);
  }
  uint64_t total_record_size = size_of_items + row.size() * sizeof(Slot) + 8;

  uint64_t total_bits = static_cast<uint32_t>(std::ceil(row.size() / 8));
  uint64_t bitmap = 0;

  for (auto it = row.begin(); it != row.end(); ++it) {
    bitmap <<= 1;
    bitmap |= (it->empty() & 1);
  }
  bitmap <<= (total_bits - row.size());

  struct Bitmap {
    uint64_t data;
  };

  struct Field {
    uint64_t *data; // data could be pointing at any data type
  };

  using RecordData = std::variant<std::monostate, Bitmap, Slot, Field>;
  RecordData *buf = new RecordData[total_record_size];
  buf[0] = Bitmap{.data = bitmap};
  uint64_t slot_pos = 1;
  uint64_t record_pos_offset = 64 * slot_pos + sizeof(Slot) * row.size();
  uint64_t slot_padding = 64 - (record_pos_offset % 64) + record_pos_offset;
  record_pos_offset += slot_padding;
  uint64_t record_pos = record_pos_offset % 64;
  string, char[], iterate all if isdigit is false ? uint64_t, int use stoi;
  (how to parse out float from string),
      float use stof for (auto it = row.begin(); it != row.end(); ++it) {
    buf[slot_pos] = Slot{.offset = slot_pos, .length = it->size()};
    buf[record_pos] = Field{.data = *it};
  }
};
