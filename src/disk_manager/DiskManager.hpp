#pragma once

#include <format>
#include <fstream>
#include <iostream>
#include <optional>

#include "Page.hpp"

struct DiskManager {

  int write(Page page) {
    auto table = std::format("{}-page", page.id);
    std::ofstream out(table, std::ios::binary);
    out.write(reinterpret_cast<const char *>(page.data.data()),
              page.data.size());
    return 0;
  };

  std::optional<Page> retrieve(uint8_t id) {
    Page page = Page(id);

    auto table = std::format("{}-page.db", id);
    std::ifstream in(table, std::ios::binary);

    if (!in.is_open()) {
      throw std::runtime_error{"File could not be opened"};
    };

    in.read(reinterpret_cast<char *>(page.data.data()), page.data.size());
    if (in.gcount() != page.data.size()) {
      throw std::runtime_error{
          "Mismatch between instream object count and data size"};
    }

    return page;
  };
};
