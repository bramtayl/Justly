#pragma once

#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <concepts>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

class QWidget;

// a subnamed should have the following method:
// static auto SubNamed::get_all_nameds() -> const QList<SubNamed>&;
struct Named {
  QString name;
  explicit Named(const char *name_input);
};

[[nodiscard]] auto get_name_or_empty(const Named *named_pointer) -> QString;

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto get_by_name(const QString &name) -> const SubNamed & {
  const auto &all_nameds = SubNamed::get_all_nameds();
  const auto named_pointer =
      std::find_if(all_nameds.cbegin(), all_nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> SubNamed>
auto substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                          const SubNamed *current_sub_named_pointer,
                          const char *default_one, const char *error_type,
                          const QString &message) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr(error_type), message);
    sub_named_pointer = &get_by_name<SubNamed>(default_one);
  }
  return *sub_named_pointer;
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto
json_field_to_named_pointer(const nlohmann::json &json_row,
                            const char *field_name) -> const SubNamed * {
  if (json_row.contains(field_name)) {
    const auto &json_named = json_row[field_name];
    Q_ASSERT(json_named.is_string());
    return &get_by_name<SubNamed>(
        QString::fromStdString(json_named.get<std::string>()));
  };
  return nullptr;
}

void add_named_to_json(nlohmann::json &json_row, const Named *named_pointer,
                       const char *column_name);

template <std::derived_from<Named> SubNamed>
auto get_named_schema(const char *description) -> nlohmann::json {
  std::vector<std::string> names;
  const auto &all_nameds = SubNamed::get_all_nameds();
  std::transform(all_nameds.cbegin(), all_nameds.cend(),
                 std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  return nlohmann::json({{"type", "string"},
                         {"description", description},
                         {"enum", std::move(names)}});
};
