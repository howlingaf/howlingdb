#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct Record {
  std::unique_ptr<uint8_t[]> data;
  uint16_t bytes;
};

struct Page {
  static constexpr uint16_t PAGE_SIZE = 4096;
  uint8_t page[PAGE_SIZE];

  static constexpr uint16_t ENTRIESp = 0;
  static constexpr uint16_t FREE_SPACE_ENDp = 2;

  static constexpr uint16_t OFFSET_BYTES = 2;
  static constexpr uint16_t LENGTH_BYTES = 2;

  Page() {
    std::memcpy(&page, &ENTRIESp, sizeof(ENTRIESp));
    std::memcpy(&page[FREE_SPACE_ENDp], &PAGE_SIZE, sizeof(FREE_SPACE_ENDp));
  }

  int insert_record(const Record record) {

    uint8_t record_insertp = free_space_endp() - record.bytes;
    std::memcpy(&page[record_insertp], record.data.get(), record.bytes);
    std::memcpy(&page[FREE_SPACE_ENDp], &record_insertp,
                sizeof(FREE_SPACE_ENDp));
    std::memcpy(&page[ENTRIESp], &page[ENTRIESp] + 1, sizeof(ENTRIESp));

    uint16_t offsetp = header_bytes() + slot_bytes();
    uint16_t lengthp = offsetp + sizeof(LENGTH_BYTES);
    std::memcpy(&page[offsetp], &record_insertp, sizeof(offsetp));
    std::memcpy(&page[lengthp], &record.bytes, sizeof(lengthp));
  }

  uint16_t free_bytes() {
    return free_space_endp() - header_bytes() - slot_bytes();
  }

private:
  uint16_t entry_count() {
    uint16_t entries;
    std::memcpy(&entries, &page[ENTRIESp], sizeof(ENTRIESp));
    return entries;
  }

private:
  uint16_t free_space_endp() {
    uint16_t free_space_end;
    std::memcpy(&free_space_end, &page[FREE_SPACE_ENDp],
                sizeof(FREE_SPACE_ENDp));
    return free_space_end;
  }

private:
  uint16_t header_bytes() { return sizeof(ENTRIESp) + sizeof(FREE_SPACE_ENDp); }

private:
  uint16_t slot_bytes() {
    return entry_count() * (sizeof(OFFSET_BYTES) + sizeof(LENGTH_BYTES));
  }
};

enum ColumnType { INT, FLOAT, VARCHAR };

struct Column {
  std::string name;
  ColumnType type;
};

struct Schema {
  std::vector<Column> columns;
};

Schema create_schema(std::vector<std::string> &keys,
                     std::vector<std::string> &first_row) {

  Schema schema;
  for (auto i = 0; i < keys.size(); i++) {
    try {
      std::stoi(first_row[i]);
      Column column;
      column.type = INT;
      column.name = first_row[i];
      schema.columns.push_back(column);
    } catch (...) {
      try {
        std::stof(first_row[i]);
        Column column;
        column.type = FLOAT;
        column.name = first_row[i];
        schema.columns.push_back(column);
      } catch (...) {
        Column column;
        column.type = VARCHAR;
        column.name = first_row[i];
        schema.columns.push_back(column);
      }
    }
  }
  return schema;
}

Record serialize(const std::vector<std::string> &fields, const Schema &schema) {
  uint16_t bitmap_bytes = (schema.columns.size() + 7) / 8;
  uint8_t *bitmap = new uint8_t[bitmap_bytes]();

  uint16_t total_bytes = 0;
  uint8_t fixed_pos = 0;
  uint8_t var_pos = bitmap_bytes;

  for (size_t i = 0; i < schema.columns.size(); ++i) {
    switch (schema.columns[i].type) {
    case INT:
      total_bytes += 4;
      var_pos += 4;
      break;
    case FLOAT:
      total_bytes += 8;
      var_pos += 8;
      break;
    case VARCHAR:
      total_bytes += 4;
      var_pos += 4;
      total_bytes += fields[i].size();
      break;
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }

  auto buf = std::make_unique<uint8_t[]>(total_bytes);
  uint8_t *write_ptr = buf.get();

  for (size_t i = 0; i < fields.size(); ++i) {
    if (fields[i].empty()) {
      bitmap[i / 8] |= 1 << (7 - (i % 8));
      continue;
    }
    switch (schema.columns[i].type) {
    case INT: {
      int int_val = std::stoi(fields[i]);
      std::memcpy(&buf[fixed_pos], &int_val, 4);
      fixed_pos += 4;
      break;
    }
    case FLOAT: {
      float fl_val = std::stof(fields[i]);
      std::memcpy(&buf[fixed_pos], &fl_val, 8);
      fixed_pos += 8;
      break;
    }
    case VARCHAR: {
      uint16_t offset = var_pos;
      uint16_t length = fields[i].size();
      std::memcpy(&buf[fixed_pos], &offset, 2);
      fixed_pos += 2;
      std::memcpy(&buf[fixed_pos], &length, 2);
      fixed_pos += 2;
      std::memcpy(&buf[var_pos], &fields[i], length);
      var_pos += length;
      break;
    }
    default:
      throw std::runtime_error("Unknown column type found");
    }
  }

  // need to move buf to record
  return Record{buf, total_bytes};
}
