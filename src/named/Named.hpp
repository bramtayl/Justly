#pragma once

#include <QString>
#include <nlohmann/json.hpp>

struct Named {
  QString name;
};

template <std::derived_from<Named> Named>
auto get_by_name(const QList<Named> &nameds,
                      const QString &name) -> const Named & {
  const auto named_pointer =
      std::find_if(nameds.cbegin(), nameds.cend(),
                   [name](const Named &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> Named> auto get_names(const QList<Named> &nameds) {
  std::vector<std::string> names;
  std::transform(
      nameds.cbegin(), nameds.cend(), std::back_inserter(names),
      [](const Named &item) -> std::string { return item.name.toStdString(); });
  return names;
}

template <std::derived_from<Named> Named>
auto json_to_named(const QList<Named> &nameds,
                  const nlohmann::json &json_instrument) -> const Named & {
  Q_ASSERT(json_instrument.is_string());
  return get_by_name(
      nameds, QString::fromStdString(json_instrument.get<std::string>()));
}

