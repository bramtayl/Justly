#pragma once

#include <QtGlobal>
#include <algorithm>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

template <typename Item>
void check_number(const std::vector<Item> &items, size_t item_number) {
  Q_ASSERT(item_number < items.size());
}

template <typename Item>
void check_end_number(const std::vector<Item> &items, size_t item_number) {
  Q_ASSERT(item_number <= items.size());
}

template <typename Item>
void check_range(const std::vector<Item> &items, size_t first_number,
                 size_t number_of_items) {
  check_number(items, first_number);
  check_end_number(items, first_number + number_of_items);
}

template <typename Item>
[[nodiscard]] auto get_const_item(const std::vector<Item> &items,
                                  size_t item_number) -> const Item & {
  check_number(items, item_number);
  return items[item_number];
}

template <typename Item>
[[nodiscard]] auto items_to_json(const std::vector<Item> &items,
                                 size_t first_number, size_t number_of_items) {
  nlohmann::json json_chords;
  check_range(items, first_number, number_of_items);
  std::transform(items.cbegin() + static_cast<int>(first_number),
                 items.cbegin() +
                     static_cast<int>(first_number + number_of_items),
                 std::back_inserter(json_chords),
                 [](const Item &item) { return item.to_json(); });
  return json_chords;
}