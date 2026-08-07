#include "layout/Page.hpp"
#include "layout/types.hpp"
#include "test_utils.hpp"
#include <cassert>

Page seed_page(vpv16 sample);
int test_insert(vpv16 sample);
int test_remove(vpv16 sample);
int test_update_var(vpv16 sample);
int test_update_fixed(vpv16 sample);

int main() {
  test_insert(sample);
  test_remove(sample);
}

/*
   TODO:
  for FIXED and VAR
    Update empty to non-empty
    Update non-empty to empty
    Update non-empty to non-empty (same size)
  Update smaller VAR to larger VAR
  Update larger VAR to smaller VAR
  Test_bitmap 1 -> 0 && 0 -> 1, N bytes
*/

int test_update_var(vpv16 sample) {

    Page page = seed_page(sample);

    const Column column = {
      .name= "name",
      .type= VARCHAR
    };

    const std::string new_name = "reddit_user_btw";
    sample[2].first = { new_name , "28", "6.9"};
    sample[2].second = 28;
    page.update(2, column, new_name);

    for (ident_t id = 0; id < (ident_t) page.entries; id++) {
      const Record record = page.get_record(id);
    std::vector<std::string> exp = sample[id].first;
    validate_record(record, exp);
  }

  std::cout << "Test Update Passed" << '\n';
  return 0;
}

int test_insert(vpv16 sample){

  const Page page = seed_page(sample);

  for (ident_t id = 0; id < (ident_t) page.entries; id++) {
    const Record record = page.get_record(id);
    std::vector<std::string> exp = sample[id].first;
    validate_record(record, exp);
  }

  std::cout << "Test Insert Passed" << '\n';
  return 0;
}

int test_remove(vpv16 sample){

  Page page = seed_page(sample);

  page.remove(0); sample.erase(sample.begin());
  page.remove(0); sample.erase(sample.begin());

  for (ident_t id = 0; id < (ident_t) page.entries; id++) {
    const Record record = page.get_record(id);
    std::vector<std::string> exp = sample[id].first;
    validate_record(record, exp);
  }

  std::cout << "Test Remove Passed" << '\n';
  return 0;
}

