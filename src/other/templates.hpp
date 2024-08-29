#pragma once

#include <QtGlobal>
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
[[nodiscard]] static auto get_item(std::vector<Item> &items,
                                   size_t item_number) -> Item & {
  check_number(items, item_number);
  return items[item_number];
}

template <typename Item>
[[nodiscard]] auto get_const_item(const std::vector<Item> &items,
                                  size_t item_number) -> const Item & {
  check_number(items, item_number);
  return items[item_number];
}
