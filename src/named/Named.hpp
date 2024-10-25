#pragma once

#include <QString>
#include <nlohmann/json.hpp>

struct Named {
  QString name;
};

template <std::derived_from<Named> SubNamed>
auto get_by_name(const QList<SubNamed> &nameds,
                      const QString &name) -> const SubNamed & {
  const auto named_pointer =
      std::find_if(nameds.cbegin(), nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> SubNamed> auto get_names(const QList<SubNamed> &nameds) {
  std::vector<std::string> names;
  std::transform(
      nameds.cbegin(), nameds.cend(), std::back_inserter(names),
      [](const SubNamed &item) -> std::string { return item.name.toStdString(); });
  return names;
}

template <std::derived_from<Named> SubNamed>
auto json_to_named(const QList<SubNamed> &nameds,
                  const nlohmann::json &json_instrument) -> const SubNamed & {
  Q_ASSERT(json_instrument.is_string());
  return get_by_name(
      nameds, QString::fromStdString(json_instrument.get<std::string>()));
}

