#include <array>
#include <cstring>

#include "types.hpp"

// ======================================================================
// Slotted page (block) layout
//
//  Block Header                              Records
//                                     EOFS---v
//  +----+----+----+----+----+----------------+----+----+----+
//  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
//  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
//             |    |    +---------------------+    |    |
//             |    +-------------------------------+    |
//             +-----------------------------------------+
//
//   #ent  = number of slot entries
//   eofs  = end-of-free-space pointer (free-space / records boundary)
//   slotK = (size, location); location = byte offset of recordK in block
//   Records pack from the end; free space shrinks toward the header.
//   slotK -> recordK. A deleted record leaves an empty, reusable slot.

//   The slot index is a stable record id: records can be compacted/moved
//   without changing ids (the directory is a level of indirection).
// ======================================================================

struct Page {

  uint8_t id = 0;
  std::array<uint8_t, PAGE_SIZE> data;
  using offset_t = uint16_t;
  using length_t = uint16_t;

  Page(uint8_t id) {
    std::memcpy(&data, &ENTRIESp, sizeof(ENTRIESp));
    std::memcpy(&data[FREE_SPACE_ENDp], &PAGE_SIZE, sizeof(PAGE_SIZE));
    this->id = id;
  };

  int insert(const Record &record) {

    uint16_t record_insertp = free_space_endp() - record.size;
    uint16_t slotp = header_bytes() + slot_bytes();
    std::memcpy(&data[record_insertp], record.data.get(), record.size);
    std::memcpy(&data[FREE_SPACE_ENDp], &record_insertp,
                sizeof(FREE_SPACE_ENDp));
    std::memcpy(&data[ENTRIESp], &(++data[ENTRIESp]), sizeof(data[ENTRIESp]));

    uint16_t offsetp = slotp;
    uint16_t lengthp = offsetp + sizeof(length_t);
    std::memcpy(&data[offsetp], &record_insertp, sizeof(record_insertp));
    std::memcpy(&data[lengthp], &record.size, sizeof(record.size));
    return 0;
  }

  uint16_t free_space() {
    return free_space_endp() - header_bytes() - slot_bytes();
  }

  uint16_t entry_count() {
    uint16_t entries;
    std::memcpy(&entries, &data[ENTRIESp], sizeof(entries));
    return entries;
  }

  uint16_t free_space_endp() {
    uint16_t free_space_end;
    std::memcpy(&free_space_end, &data[FREE_SPACE_ENDp],
                sizeof(FREE_SPACE_ENDp));
    return free_space_end;
  }

  uint16_t header_bytes() { return sizeof(ENTRIESp) + sizeof(FREE_SPACE_ENDp); }

  uint16_t slot_bytes() {
    return entry_count() * (sizeof(offset_t) + sizeof(length_t));
  }
};
