#include "types.hpp"
#include <charconv>
#include <string>
#include <system_error>
#include <vector>

// assumes the first row is a reliable source to extrapolate
Schema create_schema(std::vector<std::string> &keys,
                     std::vector<std::string> &first_row) {

  Schema schema;
  for (size_t i = 0; i < keys.size(); i++) {
    Column col;
    int result_int{};
    float result_float{};
    const auto &curr = first_row[i];
    auto start = curr.data();
    auto end = curr.data() + curr.size();

    std::from_chars_result parsed_int = std::from_chars(start, end, result_int);
    if (parsed_int.ec == std::errc() && parsed_int.ptr == end) {
      col.type = INT;
      col.name = keys[i];
      schema.columns.push_back(col);
      continue;
    }

    std::from_chars_result parsed_float =
        std::from_chars(start, end, result_float);

    if (parsed_float.ec == std::errc() && parsed_float.ptr == end) {
      col.type = FLOAT;
      col.name = keys[i];
      schema.columns.push_back(col);
      continue;
    }

    col.type = VARCHAR;
    col.name = keys[i];
    schema.columns.push_back(col);
  }

  return schema;
}
