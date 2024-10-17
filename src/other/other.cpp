#include "other/other.hpp"
#include <QtGlobal>

#include <nlohmann/json.hpp>

auto to_qsizetype(int input) -> qsizetype {
  Q_ASSERT(input >= 0);
  return static_cast<qsizetype>(input);
}

auto get_words_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"}, {"description", "the words"}});
}

auto make_validator(const char *title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
}

