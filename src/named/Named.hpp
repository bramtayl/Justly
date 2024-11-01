#pragma once

#include "other/other.hpp"
#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QStringListModel>
#include <QTextStream>
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
};

[[nodiscard]] auto get_name_or_empty(const Named *named_pointer) -> QString;

template <std::derived_from<Named> SubNamed>
[[nodiscard]] static auto get_by_name(const QList<SubNamed> &nameds,
                                      const QString &name) -> const SubNamed & {
  const auto named_pointer =
      std::find_if(nameds.cbegin(), nameds.cend(),
                   [name](const SubNamed &item) { return item.name == name; });
  Q_ASSERT(named_pointer != nullptr);
  return *named_pointer;
}

template <std::derived_from<Named> SubNamed>
auto substitute_named_for(QWidget &parent, const SubNamed *sub_named_pointer,
                          const SubNamed *current_sub_named_pointer,
                          int chord_number, int note_number,
                          const char *note_type, const char *named_type,
                          const char *default_one,
                          const char *error_type) -> const SubNamed & {
  if (sub_named_pointer == nullptr) {
    sub_named_pointer = current_sub_named_pointer;
  };
  if (sub_named_pointer == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("No ") << QObject::tr(named_type);
    add_note_location(stream, chord_number, note_number, note_type);
    stream << QObject::tr(". Using ") << QObject::tr(default_one) << ".";
    QMessageBox::warning(&parent, QObject::tr(error_type), message);
    sub_named_pointer = &get_by_name(SubNamed::get_all_nameds(), default_one);
  }
  return *sub_named_pointer;
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto
get_list_model(const QList<SubNamed> &nameds) -> QStringListModel {
  QList<QString> names({""});
  std::transform(nameds.cbegin(), nameds.cend(), std::back_inserter(names),
                 [](const SubNamed &item) {
                   return QObject::tr(item.name.toStdString().c_str());
                 });
  return QStringListModel(names);
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto
json_field_to_named_pointer(const QList<SubNamed> &nameds,
                            const nlohmann::json &json_row,
                            const char *field_name) -> const SubNamed * {
  if (json_row.contains(field_name)) {
    const auto &json_named = json_row[field_name];
    Q_ASSERT(json_named.is_string());
    return &get_by_name(nameds,
                        QString::fromStdString(json_named.get<std::string>()));
  };
  return nullptr;
}

void add_named_to_json(nlohmann::json &json_row, const Named *named_pointer,
                       const char *column_name);

template <std::derived_from<Named> SubNamed>
auto get_named_schema(const QList<SubNamed> &all_nameds,
                      const char *description) -> nlohmann::json {
  std::vector<std::string> names;
  std::transform(all_nameds.cbegin(), all_nameds.cend(),
                 std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  return nlohmann::json({{"type", "string"},
                         {"description", description},
                         {"enum", std::move(names)}});
};
