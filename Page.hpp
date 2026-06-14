#pragma once

#include <array>
#include <cstdint>

#include "src/types/layout.hpp"
#include "src/util/layout.hpp"
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
//             +-----------------------------------------// hex dumps+
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

// read write templates
// static asserts on defined types
// audit memcpy for overlaps
// audit sentinels
// layout constants
// page validation
// page_dump

struct Page {
  // TODO: overload ++ for slot increment
  std::array<uint8_t, PAGE_SIZE> buf{};
  uint8_t id = 0;
  entries_t entries = 0;
  uint16_t free_space = PAGE_SIZE - sizeof(header_t);
  offset_t free_space_offset = PAGE_SIZE;
  // TODO: have you been dirty

  // TODO: constructor to load from disk

  Page() : Page(0) {};
  Page(uint8_t id) : id(id) {
    write(buf.data(), ENTRIES_OFFSET, entries);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
  };

  int insert(const Record &record) {

    if (free_space < record.size) {
      return 1; // find another record with same catalogue (in pool or disk)
    }

    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+

    free_space_offset -= record.size;
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);

    const uint16_t start = sizeof(header_t);
    const uint16_t end = start + entries * sizeof(slot_t); // handles empty
                                                           // slots

    for (offset_t p = start; p < end; p += sizeof(slot_t)) {
      const auto offset = read<offset_t>(buf.data(), p);
      if (offset == 0) {
        uint16_t insert_offset = free_space_offset - record.size;
        write(buf.data(), insert_offset, record.data.get(), record.size);
        write(buf.data(), p, insert_offset);
        write(buf.data(), p + sizeof(offset_t), record.size);
        return 0;
      }
    }

    write(free_space_offset, record.data.get(), record.size);

    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, ++entries);

    free_space -= (record.size + sizeof(slot_t));

    write(buf.data(), start, free_space_offset);
    write(buf.data(), start + sizeof(offset_t), record.size);
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

    if (entries <= row_id) {
      return 1;
    }
    const uint16_t site = sizeof(header_t) + row_id * sizeof(slot_t);
    const auto offset = read<offset_t>(buf.data(), site, sizeof(offset_t));
    const auto length =
        read<length_t>(buf.data(), site + sizeof(offset_t), sizeof(length_t));

    // const uint16_t to_update = &data[offset];
    //  if updated_record > old_record:
    //  memcopy out all records left of old_record
    //  old_recordLeftp + (updated_record - old_record) = updated_record segment
    //  loop over buffer starting at update_recordLEFTp and memcopy back

    // walk record and to_update
    return 0;
  }

  int remove(uint8_t row_id) {
    if (entries <= row_id) {
      return 1;
    }
    const offset_t new_offset = 0;

    const uint16_t start = sizeof(header_t) + row_id * sizeof(slot_t);
    const uint16_t end = start + entries * sizeof(slot_t);

    auto offset = read<offset_t>(buf.data(), start, sizeof(offset_t));
    auto length =
        read<length_t>(buf.data(), start + sizeof(offset_t), sizeof(length_t));

    if (offset == 0) {
      return 0;
    }

    free_space += length + sizeof(slot_t);
    write(buf.data(), ENTRIES_OFFSET, --entries);
    write(buf.data(), start, new_offset);

    offset_t p = start + sizeof(slot_t);

    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+

    while (p < end) {

      const offset_t prev_offset = offset;
      const length_t prev_length = length;

      offset = read<offset_t>(buf.data(), p);
      length =
          read<length_t>(buf.data(), p + sizeof(offset_t), sizeof(length_t));
      if (offset == 0)
        break;

      const uint16_t insert_end = prev_offset + prev_length;
      const uint16_t insert_start = insert_end - length;
      write(buf.data(), insert_start, offset, length);

      offset = insert_start;
      write(buf.data(), p, offset);
      p += sizeof(slot_t);
    }
    free_space_offset = offset;
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);

    return 0;
  }
};
