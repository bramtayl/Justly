#pragma once

#include <QString>
#include <QtGlobal>
#include <concepts>

// a subnamed should have the following method:
// static auto SubNamed::get_all_nameds() -> const QList<SubNamed>&;
struct Named {
  QString name;
  explicit Named(const char *name_input);
};

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto get_by_name(const QString& name) -> const SubNamed & {
  const auto &all_nameds = SubNamed::get_all_nameds();
  const auto named_pointer =
      std::find_if(all_nameds.cbegin(), all_nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

