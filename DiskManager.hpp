#pragma once

#include "types.hpp"
#include <vector>

struct DiskManager {
  std::vector<Page> page;
  void write();
};
