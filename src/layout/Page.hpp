#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
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
  uint16_t entries = 0;
  length_t free_space = PAGE_SIZE - static_cast<length_t>(sizeof(header_t));
  offset_t free_space_offset = PAGE_SIZE;
  ident_t schema_id;

  // void print(){
  //   for (size_t si = 0; si < entries; si++ ){
  //     const offset_t off = static_cast<offset_t>(sizeof(header_t) + si * sizeof(slot_t));
  //   }
  // }
  //

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
    const length_t record_sz = static_cast<length_t>(record.size);
    if (free_space < record_sz){
      return 1; // find another record with same catalogue (in pool or disk)
    }

    const offset_t start = static_cast<uint16_t>(sizeof(header_t));
    const offset_t end = static_cast<uint16_t>(start + entries * sizeof(slot_t));
    offset_t p = start;
    const offset_t slot_sz = static_cast<offset_t>(sizeof(slot_t));
    for (; p < end; p += slot_sz ) {
      const auto off = read<offset_t>(buf.data(), p);
      if (off == 0) break;
    }

    const uint16_t insert_offset = free_space_offset - record_sz;
    write(buf.data(), insert_offset, record.data.get(), record_sz );
    write(buf.data(), p, insert_offset);
    const offset_t offset_sz = static_cast<offset_t>(sizeof(offset_t));
    const offset_t lpos = p + offset_sz;
    write(buf.data(), lpos , record_sz);

    free_space -= static_cast<uint16_t>((record_sz + sizeof(slot_t)));
    free_space_offset = insert_offset;

    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);
    write(buf.data(), ENTRIES_OFFSET, ++entries);


    write(buf.data(), start, free_space_offset);
    write(buf.data(), start + offset_sz, record_sz);
    return 0;
  }

  template <typename T> int update(const Schema& schema, const ident_t row_id, const Column &col, const T val) {
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

    const offset_t pos = static_cast<offset_t>(sizeof(header_t) + row_id * sizeof(slot_t));
    const offset_t offset = read<offset_t>(buf.data(), pos);
    const offset_t offset_sz = static_cast<offset_t>(pos+sizeof(offset_t));
    const length_t length = read<length_t>(buf.data(), pos+offset_sz);
    
    auto ptr = std::make_unique<uint8_t[]>(length);
    
    write(ptr.get(), 0, buf.data(), length);

    Record record = Record{.data=std::move(ptr), .schema=schema, .size=length};

    record.update(col, val);

    const length_t new_length = record.var_length(col);

    const std::ptrdiff_t diff = static_cast<std::ptrdiff_t>(new_length - length);

    //diff can be negative

    if ( static_cast<std::ptrdiff_t> (free_space < diff) ){} //TODO: need new page

    const offset_t end = static_cast<offset_t>((pos - sizeof(header_t))/sizeof(slot_t));
    const length_t record_len = static_cast<length_t>(record.size);


    if (diff < 0){
      const length_t ndiff = length - new_length;
      write( buf.data() , offset + ndiff, record.data.get(), record_len);
      offset_t c = static_cast<offset_t>(pos - sizeof(slot_t));
      for (;c >= end; c-= static_cast<offset_t>(sizeof(slot_t))){

        const offset_t off = read<offset_t>(buf.data(), c);
        const length_t offset_sz = static_cast<length_t>(sizeof(offset_t));
        const length_t len = read<length_t>(buf.data(), c + offset_sz);
        const offset_t data = read<offset_t>(buf.data(), off, len);

        write(buf.data(), off + ndiff, data, len);
        write(buf.data(), c, off + ndiff);
        write(buf.data(), c + offset_sz, len);
      }

      free_space += ndiff; 
      free_space_offset += ndiff; 

    } else {
      const length_t ndiff = new_length - length;
      offset_t off = offset;
      offset_t len = length;
      offset_t prev_data{};
      const length_t slot_sz = static_cast<length_t>(sizeof(slot_t));
      for ( offset_t c = pos; c > end; c-= slot_sz ) {
        const offset_t prev_off = read<offset_t>(buf.data(), pos - slot_sz);
        const length_t prev_len = read<offset_t>(buf.data(), pos + offset_sz);
        prev_data = read<offset_t>(buf.data(), prev_off, prev_len);
        write(buf.data(), off - ndiff, record.data.get(), record_len );
        write(buf.data(), pos, off - ndiff);
        write(buf.data(), offset_sz + sizeof(offset_t), record_len);
        off = prev_off; 
        len = prev_len; 
      }
      write(buf.data(), off - ndiff, record.data.get(), record_len);
      write(buf.data(), pos, off - ndiff);
      write(buf.data(), pos + sizeof(offset_t), record_len);

      free_space += ndiff; 
      free_space_offset += ndiff; 
    }
    return 0;
  }

  int remove(uint8_t row_id) {
    if (entries <= row_id) {
      return 1;
    }
    const offset_t new_offset = 0;

    const offset_t start = static_cast<offset_t>(sizeof(header_t) + row_id * sizeof(slot_t));
    const offset_t end = static_cast<offset_t>(start + entries * sizeof(slot_t));
    const offset_t offset_sz = static_cast<offset_t>(sizeof(offset_t));
    const length_t slot_sz = static_cast<offset_t>(sizeof(slot_t));

    auto offset = read<offset_t>(buf.data(), start);
    auto length = read<length_t>(buf.data(), start + offset_sz);

    if (offset == 0) return 0;

    free_space += static_cast<length_t>(length + slot_sz);
    write(buf.data(), ENTRIES_OFFSET, --entries);
    write(buf.data(), start, new_offset);

    offset_t p = static_cast<offset_t>(start + slot_sz);

    while (p < end) {

      const offset_t prev_offset = offset;
      const length_t prev_length = length;

      offset = read<offset_t>(buf.data(), p);
      length = read<length_t>(buf.data(), p + offset_sz );
      if (offset == 0) break;

      const offset_t insert_end = prev_offset + prev_length;
      const offset_t insert_start = insert_end - length;
      write(buf.data(), insert_start, offset, length);

      offset = insert_start;
      write(buf.data(), p, offset);
      p += slot_sz;
    }
    free_space_offset = offset;
    write(buf.data(), FREE_SPACE_OFFSET, free_space_offset);

    return 0;
  }


};
