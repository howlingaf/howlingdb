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
#include <vector>

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

class Page {

public:
  ident_t id;
  count_t entries = 0;
  uint16_t free_space = PAGE_SIZE - HEADER_SIZE;
  uint16_t free_space_offset = PAGE_SIZE;
  Schema schema;
  count_t slots = 0;

  const std::array<uint8_t, PAGE_SIZE> &get_buf() const { return this->buf; }

  Page(const Schema &schema, ident_t page_id = 0)
      : id(page_id), schema(schema) {
    write(buf.data(), ENTRIES_OFFSET, entries);
    write(buf.data(), SLOTS_OFFSET, slots);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
  };

  std::vector<std::pair<ident_t, std::optional<Record>>> scan() const {
    std::vector<std::pair<ident_t, std::optional<Record>>> live;
    for (ident_t id = 0; id < slots; id++) {
      std::optional<Record> record = get_record(id);
      if (record.has_value()) {
        live.emplace_back(id, std::move(*record));
      }
    }
    return live;
  };

  std::optional<Record> get_record(const ident_t row_id) const {
    const uint16_t pos =
        static_cast<uint16_t>(HEADER_SIZE + row_id * SLOT_SIZE);
    const Slot slot = Slot(pos);
    const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t len = read<uint16_t>(buf.data(), slot.length_pos);
    if (off == 0)
      return std::nullopt;
    auto ptr = std::make_unique<uint8_t[]>(len);
    write(ptr.get(), 0, &buf[off], len);

    Record record =
        Record{.buf = std::move(ptr), .schema = schema, .size = len};

    return record;
  }

  int insert(const Record &record) {
    if (free_space < record.size) {
      throw std::runtime_error{
          "Page doesn't have enough space to insert this record"};
    }
    const uint16_t end = static_cast<uint16_t>(HEADER_SIZE + SLOT_SIZE * slots);

    uint16_t pos = HEADER_SIZE;
    for (; pos < end; pos += SLOT_SIZE) {
      const Slot slot = Slot(pos);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      if (off == 0)
        break;
    }

    if (pos == end) {
      write(buf.data(), SLOTS_OFFSET, ++slots);
      free_space = static_cast<uint16_t>(free_space - SLOT_SIZE);
    }

    const Slot slot = Slot(pos);
    const uint16_t insert_offset =
        static_cast<uint16_t>(free_space_offset - record.size);

    write(buf.data(), insert_offset, record.buf.get(), record.size);
    write(buf.data(), slot.offset_pos, insert_offset);
    write(buf.data(), slot.length_pos, record.size);

    free_space = static_cast<uint16_t>((free_space - record.size));
    free_space_offset = insert_offset;
    write(buf.data(), ENTRIES_OFFSET, ++entries);
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);

    return 0;
  }

  template <typename T>
  int update(const ident_t row_id, const Column &col, const T val) {

    if (std::is_integral_v<T> && col.type != INT) {
      return 1;
    }

    if (std::is_floating_point_v<T> && col.type != FLOAT) {
      return 1;
    }

    if (std::is_constructible_v<std::string_view, T> && col.type != VARCHAR) {
      return 1;
    }

    const uint16_t site =
        static_cast<uint16_t>(HEADER_SIZE + row_id * SLOT_SIZE);

    const Slot slot = Slot(site);
    const uint16_t old_offset = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t old_length = read<uint16_t>(buf.data(), slot.length_pos);

    auto ptr = std::make_unique<uint8_t[]>(old_length);
    write(ptr.get(), 0, buf.data(), old_length);

    Record record =
        Record{.buf = std::move(ptr), .schema = schema, .size = old_length};

    record.update(col, val);
    const uint16_t end =
        static_cast<uint16_t>(HEADER_SIZE + entries * SLOT_SIZE);

    const std::ptrdiff_t diff =
        static_cast<std::ptrdiff_t>(record.size - old_length);

    if (static_cast<std::ptrdiff_t>(free_space < diff)) {
      throw std::runtime_error{
          "Page doesn't have enough space to insert this record"};
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
      for (uint16_t curr = HEADER_SIZE; curr < end; curr += SLOT_SIZE) {
        const Slot slot = Slot(curr);
        const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
        if (off == 0 && off < old_offset)
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

      for (uint16_t curr = HEADER_SIZE; curr < end; curr += SLOT_SIZE) {
        const Slot slot = Slot(curr);
        const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
        if (off == 0 && off < old_offset)
          continue;
        write(buf.data(), slot.offset_pos, static_cast<uint16_t>(off - ndiff));
      }
      free_space -= ndiff;
    }
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    return 0;
  }

  int remove(ident_t row_id) {
    if (slots < static_cast<uint16_t>(row_id + 1))
      return 1;

    const uint16_t site =
        static_cast<uint16_t>(HEADER_SIZE + SLOT_SIZE * row_id);
    const Slot slot = Slot(site);
    const uint16_t offset = read<uint16_t>(buf.data(), slot.offset_pos);
    const uint16_t length = read<uint16_t>(buf.data(), slot.length_pos);

    if (offset == 0)
      return 1;

    const uint16_t free_space_offset_0 = free_space_offset;
    const uint16_t remaining =
        static_cast<uint16_t>(offset - free_space_offset_0);
    const uint16_t free_space_offset_1 =
        static_cast<uint16_t>(free_space_offset_0 + length);

    write(buf.data(), free_space_offset_1, free_space_offset_0, remaining);
    write<uint16_t>(buf.data(), slot.offset_pos, 0);
    write<uint16_t>(buf.data(), slot.length_pos, 0);

    const uint16_t end = static_cast<uint16_t>(HEADER_SIZE + SLOT_SIZE * slots);

    for (uint16_t curr = HEADER_SIZE; curr < end; curr += SLOT_SIZE) {
      const Slot slot = Slot(curr);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      if (off != 0 && off < offset) {
        write<uint16_t>(buf.data(), slot.offset_pos,
                        static_cast<uint16_t>(off + length));
      }
    }

    free_space_offset = free_space_offset_1;
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, --entries);
    free_space = static_cast<uint16_t>(free_space + length);
    return 0;
  }

  void print() const {
    std::vector<std::vector<std::string>> rows;
    for (size_t i = 0; i < entries; i++) {
      const uint16_t pos = static_cast<uint16_t>(HEADER_SIZE + i * SLOT_SIZE);
      const Slot slot = Slot(pos);
      const uint16_t off = read<uint16_t>(buf.data(), slot.offset_pos);
      const uint16_t len = read<uint16_t>(buf.data(), slot.length_pos);
      if (off == 0)
        continue;
      auto ptr = std::make_unique<uint8_t[]>(len);
      std::memcpy(ptr.get(), &buf[off], len);
      const Record record{.buf = std::move(ptr), .schema = schema, .size = len};
      rows.push_back(record.buf_as_vector());
    }

    const size_t ncols = schema.columns.size();
    std::vector<int> widths(ncols);
    for (size_t c = 0; c < ncols; c++) {
      size_t w = schema.columns[c].name.size();
      for (const auto &row : rows)
        w = std::max(w, row[c].size());
      widths[c] = static_cast<int>(w);
    }

    auto rule = [&] {
      for (size_t c = 0; c < ncols; c++)
        std::printf("+-%.*s-", widths[c],
                    "--------------------------------------------------------");
      std::printf("+\n");
    };
    auto row = [&](const std::vector<std::string> &cells) {
      for (size_t c = 0; c < ncols; c++)
        std::printf("| %-*s ", widths[c], cells[c].c_str());
      std::printf("|\n");
    };

    std::vector<std::string> headers;
    for (const auto &col : schema.columns)
      headers.push_back(col.name);

    rule();
    row(headers);
    rule();
    for (const auto &r : rows)
      row(r);
    rule();
  }

private:
  std::array<uint8_t, PAGE_SIZE> buf{};
  void update_headers();
};
