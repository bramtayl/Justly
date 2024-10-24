#include <QString>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

template <typename Item>
void instrument_from_json(Item &item, const nlohmann::json &json_item) {
  if (json_item.contains("instrument")) {
    item.instrument_pointer =
        &json_to_item(get_all_instruments(), json_item["instrument"]);
  };
}

template <typename Item>
void percussion_set_from_json(Item &item, const nlohmann::json &json_item) {
  if (json_item.contains("percussion_set")) {
    item.percussion_set_pointer =
        &json_to_item(get_all_percussion_sets(), json_item["percussion_set"]);
  }
}

template <typename Item>
void percussion_instrument_from_json(Item &item,
                                     const nlohmann::json &json_item) {
  if (json_item.contains("percussion_instrument")) {
    item.percussion_instrument_pointer = &json_to_item(
        get_all_percussion_instruments(), json_item["percussion_instrument"]);
  }
}

template <typename Item>
void interval_from_json(Item &item, const nlohmann::json &json_item) {
  if (json_item.contains("interval")) {
    item.interval = json_to_interval(json_item["interval"]);
  }
}

template <typename Item>
void beats_from_json(Item &item, const nlohmann::json &json_item) {
  if (json_item.contains("beats")) {
    item.beats = json_to_rational(json_item["beats"]);
  }
}

template <typename Item>
void velocity_ratio_from_json(Item &item, const nlohmann::json &json_item) {
  if (json_item.contains("velocity_ratio")) {
    item.velocity_ratio = json_to_rational(json_item["velocity_ratio"]);
  }
}

template <typename Item>
void words_from_json(Item &item, const nlohmann::json &json_item) {
  if (json_item.contains("words")) {
    item.words = QString::fromStdString(json_item.value("words", ""));
  }
}

template <typename Item>
void add_named_to_json(nlohmann::json &json_item, const Item *named_pointer,
                       const char *column_name) {
  if (named_pointer != nullptr) {
    json_item[column_name] = item_to_json(*named_pointer);
  }
}

void add_interval_to_json(nlohmann::json &json_item, const Interval &interval);

void add_rational_to_json(nlohmann::json &json_item, const Rational &rational,
                          const char *column_name);

void add_words_to_json(nlohmann::json &json_item, const QString &words);
