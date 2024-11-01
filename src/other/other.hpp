#pragma once

#include <QString>
#include <QVariant>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

class QTextStream;

[[nodiscard]] auto variant_to_string(const QVariant &variant) -> QString;

[[nodiscard]] auto get_words_schema() -> nlohmann::json;

[[nodiscard]] auto get_number_schema(const char *type, const char *description,
                                     int minimum,
                                     int maximum) -> nlohmann::json;

[[nodiscard]] auto get_array_schema(const char *description, const nlohmann::json& item_json) -> nlohmann::json;
[[nodiscard]] auto get_object_schema(const char *description, const nlohmann::json& properties_json) -> nlohmann::json;

void add_words_to_json(nlohmann::json &json_row, const QString &words);

[[nodiscard]] auto
json_field_to_words(const nlohmann::json &json_row) -> QString;

void add_note_location(QTextStream &stream, int chord_number, int note_number,
                       const char *note_type);
