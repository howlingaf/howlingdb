#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <printf.h>
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

struct Page {
  std::array<uint8_t, PAGE_SIZE> buf{};
  ident_t id;
  uint16_t entries = 0;
  uint16_t free_space = PAGE_SIZE - HEADER_SIZE;
  uint16_t free_space_offset = PAGE_SIZE;
  ident_t schema_id;

  Page(ident_t schema_id, ident_t page_id = 0)
      : id(page_id), schema_id(schema_id) {
    write(buf.data(), ENTRIES_OFFSET, entries);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
  };

  void print(const Schema &schema) {
    for (size_t ci = 0; ci < entries; ci++) {
      const Slot slot = Slot(HEADER_SIZE + ci * SLOT_SIZE);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      const uint16_t len = read<uint16_t>(buf.data(), slot.length_pos);
      std::unique_ptr<uint8_t[]> ptr = std::make_unique<uint8_t[]>(len);
      std::memcpy(ptr.get(), &buf[off], len);
      Record record =
          Record{.buf = std::move(ptr), .schema = schema, .size = len};
      std::printf("\n====== Page Id:{%d}, Record Id: {%zu}, Offset: {%d}, "
                  "Length: {%d} ======\n",
                  id, ci + 1, off, len);
      record.print();
    }
  }

  int insert(const Record &record) {
    const uint16_t new_length = static_cast<uint16_t>(record.size);
    if (free_space < new_length) {
      // TODO:
      return 1;
    }
    uint16_t pos = HEADER_SIZE;
    const uint16_t fixed_data_offset_end =
        static_cast<uint16_t>(HEADER_SIZE + entries * SLOT_SIZE);

    for (; pos < fixed_data_offset_end; pos += SLOT_SIZE) {
      if (read<uint16_t>(buf.data(), pos) == 0)
        break;
    }
    Slot slot = Slot(pos);
    const uint16_t insert_offset =
        static_cast<uint16_t>(free_space_offset - record.size);
    write(buf.data(), insert_offset, record.buf.get(), record.size);
    write(buf.data(), slot.offset_pos, insert_offset);
    write(buf.data(), slot.length_pos, record.size);

    free_space -= static_cast<uint16_t>((record.size + SLOT_SIZE));
    free_space_offset = insert_offset;

    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, ++entries);

    return 0;
  }

  template <typename T>
  int update(const Schema &schema, const ident_t row_id, const Column &col,
             const T val) {
    std::cout << val << "\n";

    if (std::is_integral_v<T> && col.type != INT) {
      return 1;
    }

    if (std::is_floating_point_v<T> && col.type != FLOAT) {
      return 1;
    }

    if (std::is_constructible_v<std::string_view, T> && col.type != VARCHAR) {
      return 1;
    }

    uint16_t pos = static_cast<uint16_t>(HEADER_SIZE + row_id * SLOT_SIZE);

    Slot slot = Slot(pos);
    const uint16_t old_offset = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t old_length = read<uint16_t>(buf.data(), slot.length_pos);

    auto ptr = std::make_unique<uint8_t[]>(old_length);
    write(ptr.get(), 0, buf.data(), old_length);

    Record record =
        Record{.buf = std::move(ptr), .schema = schema, .size = old_length};

    record.update(col, val);
    const uint16_t slot_end = HEADER_SIZE + entries * SLOT_SIZE;

    const std::ptrdiff_t diff =
        static_cast<std::ptrdiff_t>(record.size - old_length);

    if (static_cast<std::ptrdiff_t>(free_space < diff)) {
    } // TODO: need new page

    if (diff < 0) {
      const uint16_t pdiff = old_length - record.size;
      write(buf.data(), old_offset, record.buf.get(), record.size);

      const uint16_t new_length =
          static_cast<uint16_t>(old_offset + record.size);

      const uint16_t new_free_space_offset =
          static_cast<uint16_t>(free_space_offset + pdiff);

      write(buf.data(), free_space_offset, new_free_space_offset, new_length);
      free_space_offset = new_free_space_offset;

      const uint16_t new_offset = static_cast<uint16_t>(old_offset + pdiff);

      write(buf.data(), slot.offset_pos, new_offset);
      write(buf.data(), slot.length_pos, record.size);

      for (pos += SLOT_SIZE; pos < slot_end; pos += SLOT_SIZE) {
        Slot slot = Slot(pos);
        const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
        if (off == 0)
          continue;
        const uint16_t new_offset = static_cast<uint16_t>(off + pdiff);
        write(buf.data(), slot.offset_pos, new_offset);
      }

      free_space += pdiff;

    } else {

      const uint16_t ndiff = static_cast<uint16_t>(record.size - old_length);
      const uint16_t new_length =
          static_cast<uint16_t>(old_offset + old_length + ndiff);
      const uint16_t new_free_space_offset =
          static_cast<uint16_t>(free_space_offset - ndiff);

      write(buf.data(), free_space_offset, new_free_space_offset, new_length);
      free_space_offset = new_free_space_offset;
      const uint16_t new_offset = static_cast<uint16_t>(old_offset - ndiff);
      write(buf.data(), new_offset, record.buf.get(), record.size);

      write(buf.data(), slot.offset_pos, new_offset);
      write(buf.data(), slot.length_pos, record.size);

      for (pos += SLOT_SIZE; pos < slot_end; pos += SLOT_SIZE) {
        Slot slot = Slot(pos);
        const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
        if (off == 0)
          continue;
        write(buf.data(), slot.offset_pos, static_cast<uint16_t>(off - ndiff));
      }
      free_space -= ndiff;
    }
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    return 0;
  }

  int remove(uint8_t row_id) {
    if (entries <= row_id) {
      return 1;
    }

    uint16_t pos = static_cast<uint16_t>(HEADER_SIZE + row_id * SLOT_SIZE);
    const uint16_t slot_end = static_cast<uint16_t>(pos + entries * SLOT_SIZE);

    Slot slot = Slot(pos);
    uint16_t old_offset = read<uint16_t>(buf.data(), slot.offset_pos);
    uint16_t old_length = read<uint16_t>(buf.data(), slot.length_pos);

    if (old_offset == 0)
      return 0;

    uint16_t new_length = static_cast<uint16_t>(free_space_offset - old_offset);
    const uint16_t new_free_space_offset =
        static_cast<uint16_t>(free_space_offset + old_length);

    write(buf.data(), free_space_offset, new_free_space_offset, new_length);
    write(buf.data(), slot.offset_pos, 0);
    write(buf.data(), slot.length_pos, 0);
    free_space_offset = new_free_space_offset;

    for (pos += SLOT_SIZE; pos < slot_end; pos += SLOT_SIZE) {
      Slot slot = Slot(pos);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      if (off == 0)
        continue;
      write(buf.data(), slot.offset_pos,
            static_cast<uint16_t>(off + old_length));
    }
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, --entries);
    return 0;
  }
};
