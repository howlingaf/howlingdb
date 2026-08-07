#pragma once
#include "layout/types.hpp"
#include "layout/Record.hpp"
#include "layout/Page.hpp"
#include <cassert>
#include <vector>

typedef std::vector<std::pair<std::vector<std::string>, uint16_t>> vpv16;

const Schema schema {
  .columns = {
    {.name = "name",   .type = VARCHAR},
    {.name = "age",   .type = INT},
    {.name = "height",   .type = FLOAT},
  },
  .id = 1,
};

const vpv16 sample = {
  {{"howlingaf", "32", "5.8"}, 22},
  {{"hairyrugaf", "5", "1.9"}, 23},
  {{"twitch_user", "28", "6.9"}, 24},
  {{"youtube_user", "19", "6.8"},25},
  {{"instatgram_user", "22", "6.7"}, 28},
  {{"", "22", "6.7"}, 13},
};

inline int validate_record(const Record& record, std::vector<std::string>& expected) {
  std::vector<std::string> attrs = record.cells();
  if (attrs.size() != expected.size()) return 1;
  const size_t n = expected.size();
  for (size_t i = 0; i < n; i++) {
    if (record.schema.columns[i].type == FLOAT){
      expected[i] = std::format("{:.2f}", std::stof(expected[i]));
      attrs[i] = std::format("{:.2f}", std::stof(attrs[i]));
    }
    assert(attrs[i] == expected[i]);
  }
  return 0;
}

inline int validate_page(const Page& page, uint16_t& free_space, uint16_t& entries, uint16_t& free_space_offset) {
  assert(page.free_space == free_space);
  assert(page.entries == entries);
  assert(page.free_space_offset == free_space_offset);
  return 0;
}


inline Page seed_page(vpv16 sample){
  Page page = Page(schema); 

  uint16_t free_space = static_cast<uint16_t>(PAGE_SIZE - HEADER_SIZE);
  uint16_t entries = 0;
  uint16_t free_space_offset = PAGE_SIZE;

  validate_page(page, free_space, entries, free_space_offset);
  for (uint16_t i = 0; i < (uint16_t) sample.size(); i++){
    const Record record = create_record(sample[i].first, schema);
    page.insert(record);
    const uint16_t length = sample[i].second;
    free_space = static_cast<uint16_t>(free_space - length - SLOT_SIZE);
    free_space_offset = static_cast<uint16_t>(free_space_offset - length);
    validate_page(page, free_space, ++entries, free_space_offset);
  }
  return page;
}


