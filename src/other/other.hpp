#pragma once

#include <QString>
#include <QtGlobal>

#include <iterator>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

template <typename T> class QList;

template <typename Item>
auto get_names(const QList<Item> &items) {
  std::vector<std::string> names;
  std::transform(
      items.cbegin(), items.cend(), std::back_inserter(names),
      [](const Item &item) -> std::string { return item.name.toStdString(); });
  return names;
}

template <typename Item>
auto get_item_by_name(const QList<Item> & all_items, const QString &name) -> const Item & {
  const auto item_pointer =
      std::find_if(all_items.cbegin(), all_items.cend(),
                   [name](const Item &item) {
                     return item.name == name;
                   });
  Q_ASSERT(item_pointer != nullptr);
  return *item_pointer;
}

template <typename Item>
auto json_to_item(const QList<Item>& all_items, const nlohmann::json &json_instrument)
    -> const Item & {
  Q_ASSERT(json_instrument.is_string());
  return get_item_by_name(all_items,
      QString::fromStdString(json_instrument.get<std::string>()));
}

template <typename Item>
[[nodiscard]] auto item_to_json(const Item& item) -> nlohmann::json {
  return item.name.toStdString();
};

[[nodiscard]] auto get_words_schema() -> nlohmann::json;

[[nodiscard]] auto make_validator(const char *title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;

