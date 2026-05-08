#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stddef.h>

#include "DiskManager.hpp"
#include "types.hpp"

using offset_t = uint16_t;
using length_t = uint16_t;

struct PageManager {

  DiskManager disk_manager;
  Page page;

  PageManager(DiskManager disk_manager) {};
  uint8_t insert(const Record &record) {
    if (free_bytes() < record.bytes) {
      std::cout << "Not enough space!/n";
      // create page
      // copy page content on to disk (just raw)
      // move_to_disk_manager(std::move(&page));
      // empty out page
      return 1;
    };
    uint16_t record_insertp = free_space_endp() - record.bytes;
    std::memcpy(&page.data[record_insertp], record.data.get(), record.bytes);
    std::memcpy(&page.data[FREE_SPACE_ENDp], &record_insertp,
                sizeof(FREE_SPACE_ENDp));
    uint16_t count = entry_count();
    std::memcpy(&page.data[ENTRIESp], &(++count), sizeof count);

    uint16_t offsetp = header_bytes() + slot_bytes();
    uint16_t lengthp = offsetp + sizeof(length_t);
    std::memcpy(&page.data[offsetp], &record_insertp, sizeof(offsetp));
    std::memcpy(&page.data[lengthp], &record.bytes, sizeof(lengthp));
    return 0;
  }

  uint16_t free_bytes() {
    return free_space_endp() - header_bytes() - slot_bytes();
  }
  uint16_t entry_count() {
    uint16_t entries;
    std::memcpy(&entries, &page.data[ENTRIESp], sizeof(ENTRIESp));
    return entries;
  }
  uint16_t free_space_endp() {
    uint16_t free_space_end;
    std::memcpy(&free_space_end, &page.data[FREE_SPACE_ENDp],
                sizeof(FREE_SPACE_ENDp));
    return free_space_end;
  }
  uint16_t header_bytes() { return sizeof(ENTRIESp) + sizeof(FREE_SPACE_ENDp); }
  uint16_t slot_bytes() {
    return entry_count() * (sizeof(offset_t) + sizeof(length_t));
  }

  // void move_to_disk_manager();
};
