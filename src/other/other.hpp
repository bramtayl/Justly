#pragma once

#include <QtGlobal>

#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

template <typename T> class QList;

[[nodiscard]] auto to_qsizetype(int input) -> qsizetype;

template <typename Item>
auto get_names(const QList<Item> &items) {
  std::vector<std::string> names;
  std::transform(
      items.cbegin(), items.cend(), std::back_inserter(names),
      [](const Item &item) -> std::string { return item.name.toStdString(); });
  return names;
}

[[nodiscard]] auto get_words_schema() -> nlohmann::json;

