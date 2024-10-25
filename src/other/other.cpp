#include "other/other.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>

auto variant_to_string(const QVariant &variant) -> QString {
  Q_ASSERT(variant.canConvert<QString>());
  return variant.value<QString>();
}

auto get_words_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"}, {"description", "the words"}});
}

void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
}

auto make_validator(const char *title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
}
