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

