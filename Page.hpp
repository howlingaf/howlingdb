#include <array>
#include <cstdint>
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
// theres enough space, add it there

//   The slot index is a stable record id: records can be compacted/moved
//   without changing ids (the directory is a level of indirection).
// ======================================================================

constexpr uint16_t PAGE_SIZE = 4096;

constexpr uint8_t ENTRIES_OFFSET = 0;
constexpr uint8_t FREE_SPACE_OFFSET = 2;

using header_t = uint32_t;
using entries_t = uint16_t;
using free_space_t = uint16_t;

using slot_t = uint32_t;
using offset_t = uint16_t;
using length_t = uint16_t;

struct Page {

  std::array<uint8_t, PAGE_SIZE> data;
  uint8_t id = 0;
  uint16_t entries = 0;
  uint16_t free_space = PAGE_SIZE - sizeof(header_t);
  uint16_t free_space_offset = PAGE_SIZE;

  // TODO: constructor to load from disk
  Page() : Page(0) {};
  Page(uint8_t id) : id(id) {
    std::memcpy(&data[ENTRIES_OFFSET], &entries, sizeof(entries_t));
    std::memcpy(&data[FREE_SPACE_OFFSET], &free_space_offset,
                sizeof(free_space_t));
  };

  int insert(const Record &record) {
    // TODO: handle not enough space
    // TODO: guard for dummy slots

    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+

    free_space_offset -= record.size;
    const uint16_t start = sizeof(header_t) + entries * sizeof(slot_t);
    std::memcpy(&data[free_space_offset], record.data.get(), record.size);

    free_space -= record.size + sizeof(slot_t);

    std::memcpy(&data[FREE_SPACE_OFFSET], &free_space_offset,
                sizeof(free_space_t));
    std::memcpy(&data[ENTRIES_OFFSET], &(++entries), sizeof(entries_t));

    const offset_t offset = start;
    const length_t length = offset + sizeof(offset);
    std::memcpy(&data[offset], &free_space_offset, sizeof(offset_t));
    std::memcpy(&data[length], &record.size, sizeof(length_t));
    return 0;
  }

  int update(uint8_t row_id, Record record) {

    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+

    // if (record.size > free_space()) {
    //  find a page of the same schema and has enough space
    //  else create a new Page for it
    //  return 0;
    //}

    // enough space
    // is the record buffer we're updating greater,less, same
    // if same replace
    //
    //
    // if updated_record < old_record:
    // rewrite record from right to left
    // loop over all records to the left and memcopy them starting at the start
    // of the updated record
    //
    //
    // 1..if i delete the record,
    // 2. shift remaining records,
    // 3. recalculate slots
    // blast radius would be contained within the page

    offset_t offset;
    length_t length;
    if (entries <= row_id) {
      return 1;
    }
    const uint16_t site = sizeof(header_t) + row_id * sizeof(slot_t);
    std::memcpy(&offset, &data[site], sizeof(offset_t));
    std::memcpy(&length, &data[site + sizeof(offset_t)], sizeof(length_t));

    const uint8_t *to_update = &data[offset];
    // if updated_record > old_record:
    // memcopy out all records left of old_record
    // old_recordLeftp + (updated_record - old_record) = updated_record segment
    // loop over buffer starting at update_recordLEFTp and memcopy back

    // walk record and to_update
    return 0;
  }

  int remove(uint8_t row_id) {
    if (entries <= row_id) {
      return 1;
    }
    const offset_t new_offset = 0;
    offset_t offset;
    length_t length;

    const uint16_t start = sizeof(header_t) + row_id * sizeof(slot_t);
    const uint16_t end = start + entries * sizeof(slot_t);

    std::memcpy(&offset, &data[start], sizeof(offset_t));
    std::memcpy(&length, &data[start + sizeof(offset_t)], sizeof(length_t));

    if (offset == 0) {
      return 0;
    }

    free_space += length + sizeof(slot_t);
    std::memcpy(&data[ENTRIES_OFFSET], &(--entries), sizeof(entries_t));
    std::memcpy(&data[start], &new_offset, sizeof(offset_t));

    uint16_t p = start + sizeof(slot_t);

    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+

    while (p < end) {
      uint16_t temp;
      std::memcpy(&temp, &data[p], sizeof(offset_t));
      if (temp != 0) {

        uint16_t prev_offset;
        uint16_t prev_length;

        std::memcpy(&prev_offset, &offset, sizeof(offset_t));
        std::memcpy(&prev_length, &length, sizeof(length_t));

        std::memcpy(&offset, &data[p], sizeof(offset_t));
        std::memcpy(&length, &data[p + sizeof(offset_t)], sizeof(length_t));

        const uint16_t insert_end = prev_offset + prev_length;
        const uint16_t insert_start = insert_end - length;
        std::memmove(&data[insert_start], &data[offset], length);

        offset = insert_start;
        std::memcpy(&data[p], &offset, sizeof(offset_t));
      }
      p += sizeof(slot_t);
    }
    free_space_offset = offset;
    std::memcpy(&data[FREE_SPACE_OFFSET], &free_space_offset, sizeof(offset_t));

    return 0;
  }
};
