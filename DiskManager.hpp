#pragma once

#include <format>
#include <fstream>
#include <iostream>

#include "Page.hpp"

struct DiskManager {

  int write(Page page) {
    auto table = std::format("{}-page", page.id);
    std::ofstream out(table, std::ios::binary);
    out.write(reinterpret_cast<const char *>(page.data.data()),
              page.data.size());
    return 0;
  };

  Page fetch(uint8_t id) {
    Page page = Page(id);
    auto table = std::format("{}-page", id);
    std::ifstream in(table, std::ios::binary);
    in.read(reinterpret_cast<char *>(page.data.data()), page.data.size());
    return page;
  };
};
