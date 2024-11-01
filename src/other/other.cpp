#include "other/other.hpp"

#include <QObject>
#include <QTextStream>
#include <QtGlobal>
#include <nlohmann/json.hpp>

auto variant_to_string(const QVariant &variant) -> QString {
  Q_ASSERT(variant.canConvert<QString>());
  return variant.value<QString>();
}

auto get_words_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"}, {"description", "the words"}});
}

auto get_number_schema(const char *type, const char *description, int minimum,
                       int maximum) -> nlohmann::json {
  return nlohmann::json({{"type", type},
                         {"description", description},
                         {"minimum", minimum},
                         {"maximum", maximum}});
}

auto get_array_schema(const char *description, const nlohmann::json& item_json) -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", description},
       {"items", item_json}});
};

auto get_object_schema(const char *description, const nlohmann::json& properties_json) -> nlohmann::json {
  return nlohmann::json(
      {{"type", "object"},
       {"description", description},
       {"properties", properties_json}});
};

void add_int_to_json(nlohmann::json& json_object, const char* field_name, int value, int default_value) {
  if (value != default_value) {
    json_object[field_name] = value;
  }
}

void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
}

auto json_field_to_words(const nlohmann::json &json_row) -> QString {
  if (json_row.contains("words")) {
    return QString::fromStdString(json_row["words"]);
  }
  return "";
}

void add_note_location(QTextStream &stream, int chord_number, int note_number,
                       const char *note_type) {
  stream << QObject::tr(" for chord ") << chord_number + 1 << QObject::tr(", ")
         << QObject::tr(note_type) << QObject::tr(" note ") << note_number + 1;
}
