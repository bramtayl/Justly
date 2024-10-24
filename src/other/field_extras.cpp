#include "other/field_extras.hpp"

#include "interval/Interval.hpp"
#include "rational/Rational.hpp"
#include <nlohmann/json.hpp>

void add_interval_to_json(nlohmann::json &json_item, const Interval &interval) {
  if (!interval_is_default(interval)) {
    json_item["interval"] = interval_to_json(interval);
  }
}

void add_rational_to_json(nlohmann::json &json_item, const Rational &rational,
                          const char *column_name) {
  if (!(rational_is_default(rational))) {
    json_item[column_name] = rational_to_json(rational);
  }
}

void add_words_to_json(nlohmann::json &json_item, const QString &words) {
  if (!words.isEmpty()) {
    json_item["words"] = words.toStdString().c_str();
  }
}
