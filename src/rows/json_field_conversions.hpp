#include <QString>
#include <concepts>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "named/Named.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

struct Row;

template <std::derived_from<Row> Row>
void instrument_from_json(Row &row, const nlohmann::json &json_row) {
  if (json_row.contains("instrument")) {
    row.instrument_pointer =
        &json_to_named(get_all_instruments(), json_row["instrument"]);
  };
}

template <std::derived_from<Row> Row>
void percussion_set_pointer_from_json(Row &row, const nlohmann::json &json_row) {
  if (json_row.contains("percussion_set")) {
    row.percussion_set_pointer =
        &json_to_named(get_all_percussion_sets(), json_row["percussion_set"]);
  }
}

template <std::derived_from<Row> Row>
void percussion_instrument_pointer_from_json(Row &row,
                                     const nlohmann::json &json_row) {
  if (json_row.contains("percussion_instrument")) {
    row.percussion_instrument_pointer = &json_to_named(
        get_all_percussion_instruments(), json_row["percussion_instrument"]);
  }
}

template <std::derived_from<Row> Row>
void interval_from_json(Row &row, const nlohmann::json &json_row) {
  if (json_row.contains("interval")) {
    row.interval = json_to_interval(json_row["interval"]);
  }
}

template <std::derived_from<Row> Row>
void beats_from_json(Row &row, const nlohmann::json &json_row) {
  if (json_row.contains("beats")) {
    row.beats = json_to_rational(json_row["beats"]);
  }
}

template <std::derived_from<Row> Row>
void velocity_ratio_from_json(Row &row, const nlohmann::json &json_row) {
  if (json_row.contains("velocity_ratio")) {
    row.velocity_ratio = json_to_rational(json_row["velocity_ratio"]);
  }
}

template <std::derived_from<Row> Row>
void words_from_json(Row &row, const nlohmann::json &json_row) {
  if (json_row.contains("words")) {
    row.words = QString::fromStdString(json_row.value("words", ""));
  }
}

template <std::derived_from<Named> Named>
void add_named_to_json(nlohmann::json &json_row, const Named *named_pointer,
                       const char* column_name) {
  if (named_pointer != nullptr) {
    json_row[column_name] = named_pointer->name.toStdString();
  }
}

void add_interval_to_json(nlohmann::json &json_row, const Interval &interval);

void add_rational_to_json(nlohmann::json &json_row, const Rational &rational,
                          const char *column_name);

void add_words_to_json(nlohmann::json &json_row, const QString &words);
