#pragma once

#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

struct Page {
  static constexpr uint16_t PAGE_SIZE = 4096;
  static constexpr uint16_t ENTRIESp = 0;
  static constexpr uint16_t FREE_SPACE_ENDp = 2;
  static constexpr uint16_t SLOT_START = 4;

  using offset_t = uint16_t;
  using length_t = uint16_t;

  uint8_t page[PAGE_SIZE];

  Page() {
    std::memcpy(&page, &ENTRIESp, sizeof(ENTRIESp));
    std::memcpy(&page[FREE_SPACE_ENDp], &PAGE_SIZE, sizeof(PAGE_SIZE));
  }

  uint8_t insert_record(const Record &record) {
    if (free_bytes() < record.bytes) {
      std::cout << "Not enough space!/n";
      return 1;
    };
    uint16_t record_insertp = free_space_endp() - record.bytes;
    std::memcpy(&page[record_insertp], record.data.get(), record.bytes);
    std::memcpy(&page[FREE_SPACE_ENDp], &record_insertp,
                sizeof(FREE_SPACE_ENDp));
    uint16_t count = entry_count();
    std::memcpy(&page[ENTRIESp], &(++count), sizeof count);

    uint16_t offsetp = header_bytes() + slot_bytes();
    uint16_t lengthp = offsetp + sizeof(length_t);
    std::memcpy(&page[offsetp], &record_insertp, sizeof(offsetp));
    std::memcpy(&page[lengthp], &record.bytes, sizeof(lengthp));
    return 0;
  }

  uint8_t delete_record(const uint16_t id) {
    uint8_t items = *page;
    for (size_t i = SLOT_START;
         i < entry_count() * sizeof(offset_t) + sizeof(length_t); i = i + 4) {
    }

    return 0;
  };

  // TODO: delete record

private:
  uint16_t free_bytes() {
    return free_space_endp() - header_bytes() - slot_bytes();
  }
  uint16_t entry_count() {
    uint16_t entries;
    std::memcpy(&entries, &page[ENTRIESp], sizeof(ENTRIESp));
    return entries;
  }
  uint16_t free_space_endp() {
    uint16_t free_space_end;
    std::memcpy(&free_space_end, &page[FREE_SPACE_ENDp],
                sizeof(FREE_SPACE_ENDp));
    return free_space_end;
  }
  uint16_t header_bytes() { return sizeof(ENTRIESp) + sizeof(FREE_SPACE_ENDp); }
  uint16_t slot_bytes() {
    return entry_count() * (sizeof(offset_t) + sizeof(length_t));
  }
};
