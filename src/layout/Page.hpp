#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "Record.hpp"
#include "types.hpp"
#include "utils.hpp"

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
  std::array<uint8_t, PAGE_SIZE> buf{};
  ident_t id = 0;
  entries_t entries = 0;
  uint16_t free_space = PAGE_SIZE - sizeof(header_t);
  offset_t free_space_offset = PAGE_SIZE;
  ident_t schema_id;
  // TODO: have you been dirty

  // TODO: constructor to load from disk

  Page(ident_t schema_id) : schema_id(schema_id) {
    write(buf.data(), ENTRIES_OFFSET, entries);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
  };

  Page(ident_t page_id, ident_t schema_id) : id(page_id), schema_id(schema_id) {
    write(buf.data(), ENTRIES_OFFSET, entries);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
  };


    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+


  int insert(const Record &record) {
    if (free_space < record.size) {
      return 1; // find another record with same catalogue (in pool or disk)
    }

    const offset_t start = sizeof(header_t);
    const offset_t end = start + entries * sizeof(slot_t); // handles empty
                                                           // slots
    offset_t p = start;
    for (; p < end; p += sizeof(slot_t)) {
      const auto offset = read<offset_t>(buf.data(), p);
      if (offset == 0) break;
    }

    const uint16_t insert_offset = free_space_offset - record.size;
    write(buf.data(), insert_offset, record.data.get(), record.size);
    write(buf.data(), p, insert_offset);
    write(buf.data(), p + sizeof(offset_t), record.size);

    free_space -= (record.size + sizeof(slot_t));
    free_space_offset = insert_offset;

    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, ++entries);


    write(buf.data(), start, free_space_offset);
    write(buf.data(), start + sizeof(offset_t), record.size);
    return 0;
  }

  // TODO: Page::update
  template <typename T> int update(const Schema& schema, const ident_t row_id, const Column &col, const T val) {
    std::cout << val << "\n";

    //  Block Header                              Records
    //                                     EOFS---v
    //  +----+----+----+----+----+----------------+----+----+----+
    //  |#ent|eofs| s0 | s1 | s2 |   free space   | r2 | r1 | r0 |
    //  +----+----+-|--+-|--+-|--+----------------+-^--+-^--+-^--+
    //             |    |    +---------------------+    |    |
    //             |    +-------------------------------+    |
    //             +-----------------------------------------+

    if (std::is_integral_v<T> && col.type != INT) {
      return 1;
    }

    if (std::is_floating_point_v<T> && col.type != FLOAT) {
      return 1;
    }

    if (std::is_constructible_v<std::string_view, T> && col.type != VARCHAR) {
      return 1;
    }


    const offset_t p  = sizeof(header_t) + row_id * sizeof(slot_t);
    const offset_t offset = read<offset_t>(buf.data(), p);
    const length_t length = read<length_t>(buf.data(), p+sizeof(offset_t));

    std::unique_ptr<uint8_t[]> data = read<std::unique_ptr<uint8_t[]>>(buf.data(),offset,length);
    Record record = Record{.data=std::move(data), .schema=schema, .size=length};

    const length_t add = record.var_length(col);
    if (length + add > PAGE_SIZE){} // need new page
    record.update(col, val);
    //TODO: still need to resize and update page if its it fits
    return 0;
  }

  int remove(uint8_t row_id) {
    if (entries <= row_id) {
      return 1;
    }
    const offset_t new_offset = 0;

    const uint16_t start = sizeof(header_t) + row_id * sizeof(slot_t);
    const uint16_t end = start + entries * sizeof(slot_t);

    auto offset = read<offset_t>(buf.data(), start);
    auto length = read<length_t>(buf.data(), start + sizeof(offset_t));

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
      length = read<length_t>(buf.data(), p + sizeof(offset_t));
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
