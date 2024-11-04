#pragma once

#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <nlohmann/json.hpp>

class QTextStream;

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

template <typename Thing>
[[nodiscard]] static auto
get_const_reference(const Thing *thing_pointer) -> const Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

[[nodiscard]] auto variant_to_string(const QVariant &variant) -> QString;

[[nodiscard]] auto get_words_schema() -> nlohmann::json;

[[nodiscard]] auto get_number_schema(const char *type, const char *description,
                                     int minimum,
                                     int maximum) -> nlohmann::json;

[[nodiscard]] auto
get_array_schema(const char *description,
                 const nlohmann::json &item_json) -> nlohmann::json;
[[nodiscard]] auto
get_object_schema(const char *description,
                  const nlohmann::json &properties_json) -> nlohmann::json;

void add_int_to_json(nlohmann::json &json_object, const char *field_name,
                     int value, int default_value);
void add_words_to_json(nlohmann::json &json_row, const QString &words);

[[nodiscard]] auto
json_field_to_words(const nlohmann::json &json_row) -> QString;

void add_note_location(QTextStream &stream, int chord_number, int note_number,
                       const char *note_type);
