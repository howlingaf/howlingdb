#pragma once

#include "types.hpp"
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>

struct Page {
  static constexpr uint16_t PAGE_SIZE = 4096;
  uint8_t page[PAGE_SIZE];

  static constexpr uint16_t ENTRIESp = 0;
  static constexpr uint16_t FREE_SPACE_ENDp = 2;

  static constexpr uint16_t OFFSET_BYTES = 2;
  static constexpr uint16_t LENGTH_BYTES = 2;

  Page() {
    std::memcpy(&page, &ENTRIESp, sizeof(ENTRIESp));
    std::memcpy(&page[FREE_SPACE_ENDp], &PAGE_SIZE, sizeof(FREE_SPACE_ENDp));
  }

  int insert_record(const Record record) {
    // TODO: check if enough space
    uint8_t record_insertp = free_space_endp() - record.bytes;
    std::memcpy(&page[record_insertp], record.data.get(), record.bytes);
    std::memcpy(&page[FREE_SPACE_ENDp], &record_insertp,
                sizeof(FREE_SPACE_ENDp));
    std::memcpy(&page[ENTRIESp], &page[ENTRIESp] + 1, sizeof(ENTRIESp));

    uint16_t offsetp = header_bytes() + slot_bytes();
    uint16_t lengthp = offsetp + sizeof(LENGTH_BYTES);
    std::memcpy(&page[offsetp], &record_insertp, sizeof(offsetp));
    std::memcpy(&page[lengthp], &record.bytes, sizeof(lengthp));
    return 0;
  }
  // TODO: delete record

private:
  uint16_t free_bytes() {
    return free_space_endp() - header_bytes() - slot_bytes();
  }

private:
  uint16_t entry_count() {
    uint16_t entries;
    std::memcpy(&entries, &page[ENTRIESp], sizeof(ENTRIESp));
    return entries;
  }

private:
  uint16_t free_space_endp() {
    uint16_t free_space_end;
    std::memcpy(&free_space_end, &page[FREE_SPACE_ENDp],
                sizeof(FREE_SPACE_ENDp));
    return free_space_end;
  }

private:
  uint16_t header_bytes() { return sizeof(ENTRIESp) + sizeof(FREE_SPACE_ENDp); }

private:
  uint16_t slot_bytes() {
    return entry_count() * (sizeof(OFFSET_BYTES) + sizeof(LENGTH_BYTES));
  }
};
