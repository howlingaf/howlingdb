#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <printf.h>
#include <stdexcept>
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
  const Schema schema;

  Page(const Schema& schema, ident_t page_id = 0)
      : id(page_id), schema(schema) {
    write(buf.data(), ENTRIES_OFFSET, entries);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
  };
  
  Record get_record(const ident_t row_id) const {

    const uint16_t pos = static_cast<uint16_t>(HEADER_SIZE + row_id * SLOT_SIZE);
    const Slot slot = Slot(pos);
    const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t len = read<uint16_t>(buf.data(), slot.length_pos);

    auto ptr = std::make_unique<uint8_t[]>(len);
    write(ptr.get(), 0, &buf[off], len);

    Record record =
        Record{.buf = std::move(ptr), .schema = schema, .size = len};
    
    return record;
  }

  template <typename T> T
  read_val (const Record& record, ident_t col_id) {
      const uint16_t pos = static_cast<uint16_t>(HEADER_SIZE + SLOT_SIZE * col_id);
      int val;

      switch (record.schema.columns[col_id].type) {
        case INT: {
        val = read<int>(record.buf.get(), pos);
      }

      case FLOAT:{
        const float val = read<float>(record.buf.get(), pos);
        return val; 
        break;
      }

      case VARCHAR:{
        const Slot slot = Slot(pos);
        const uint16_t off = read<uint16_t>(record.buf.get(), slot.offset_pos);
        const uint16_t len = read<uint16_t>(record.buf.get(), slot.length_pos);
        const auto val = read(record.buf.get(), off, len);
        return val;
      }

      default:
        break;
      }
      return val; 
  };

  void print() {
    std::vector<std::vector<std::string>> rows;
    for (uint16_t ci = 0; ci < entries; ci++) {
      const uint16_t pos = static_cast<uint16_t>(HEADER_SIZE + ci * SLOT_SIZE);
      const Slot slot = Slot(pos);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      const uint16_t len = read<uint16_t>(buf.data(), slot.length_pos);
      if (off == 0) continue;
      auto ptr = std::make_unique<uint8_t[]>(len);
      std::memcpy(ptr.get(), &buf[off], len);
      const Record record{.buf = std::move(ptr), .schema = schema, .size = len};
      rows.push_back(record.cells());
    }

    const size_t ncols = schema.columns.size();
    std::vector<int> widths(ncols);
    for (size_t c = 0; c < ncols; c++) {
      size_t w = schema.columns[c].name.size();
      for (const auto& row : rows)
        w = std::max(w, row[c].size());
      widths[c] = static_cast<int>(w);
    }

    auto rule = [&] {
      for (size_t c = 0; c < ncols; c++)
        std::printf("+-%.*s-", widths[c],
                    "--------------------------------------------------------");
      std::printf("+\n");
    };
    auto row = [&](const std::vector<std::string>& cells) {
      for (size_t c = 0; c < ncols; c++)
        std::printf("| %-*s ", widths[c], cells[c].c_str());
      std::printf("|\n");
    };

    std::vector<std::string> headers;
    for (const auto& col : schema.columns)
      headers.push_back(col.name);

    rule();
    row(headers);
    rule();
    for (const auto& r : rows)
      row(r);
    rule();
  }

  int insert(const Record &record) {
    const uint16_t new_length = static_cast<uint16_t>(record.size);
    if (free_space < new_length) {
      throw std::runtime_error{"Page doesn't have enough space to insert this record"};
    }
    uint16_t pos = HEADER_SIZE;
    const uint16_t fixed_data_offset_end =
        static_cast<uint16_t>(HEADER_SIZE + entries * SLOT_SIZE);
    for (; pos < fixed_data_offset_end; pos += SLOT_SIZE) {
      if (read<uint16_t>(buf.data(), pos) == 0)
        break;
    }
    const Slot slot = Slot(pos);
    const uint16_t insert_offset =
        static_cast<uint16_t>(free_space_offset - record.size);
    write(buf.data(), insert_offset, record.buf.get(), record.size);
    write(buf.data(), slot.offset_pos, insert_offset);
    write(buf.data(), slot.length_pos, record.size);

    free_space -= static_cast<uint16_t>((record.size + SLOT_SIZE));
    free_space_offset = insert_offset;
    write(buf.data(), FREE_SPACE_OFFSET,  free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, ++entries);

    return 0;
  }

  template <typename T>
  int update(const ident_t row_id, const Column &col,
             const T val) {

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

    const Slot slot = Slot(pos);
    const uint16_t old_offset = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t old_length = read<uint16_t>(buf.data(), slot.length_pos);

    auto ptr = std::make_unique<uint8_t[]>(old_length);
    write(ptr.get(), 0, buf.data(), old_length);

    Record record =
        Record{.buf = std::move(ptr), .schema = schema, .size = old_length};

    record.update(col, val);
    const uint16_t slot_end = static_cast<uint16_t>(HEADER_SIZE + entries * SLOT_SIZE);

    const std::ptrdiff_t diff =
        static_cast<std::ptrdiff_t>(record.size - old_length);

    if (static_cast<std::ptrdiff_t>(free_space < diff)) {
      throw std::runtime_error{"Page doesn't have enough space to insert this record"};
    } 

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
        const Slot slot = Slot(pos);
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
        const Slot slot = Slot(pos);
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
    const uint16_t end_0 = static_cast<uint16_t>(HEADER_SIZE + entries * SLOT_SIZE);

    const Slot slot = Slot(pos);
    const uint16_t offset = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t length = read<uint16_t>(buf.data(), slot.length_pos);

    const uint16_t free_space_offset_0 = free_space_offset;
    const uint16_t leftover_length = static_cast<uint16_t>(offset - free_space_offset_0);
    const uint16_t free_space_offset_1 =
        static_cast<uint16_t>(free_space_offset_0 + length);

    write(buf.data(), free_space_offset_1, free_space_offset_0, leftover_length);
    const uint16_t next_pos = pos + SLOT_SIZE;
    write(buf.data(),pos, next_pos , end_0 - next_pos);
    const uint16_t end_1 = static_cast<uint16_t>(end_0 - SLOT_SIZE);
    free_space_offset = free_space_offset_1;

    for (;pos < end_1; pos += SLOT_SIZE) {
      const Slot slot = Slot(pos);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      write(buf.data(), slot.offset_pos,
            static_cast<uint16_t>(off + length));
    }

    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, --entries);
    free_space = static_cast<uint16_t>(free_space - length - SLOT_SIZE);
    return 0;
  }
};
