#include "row/Row.hpp"

#include <algorithm>
#include <vector>

#include "abstract_rational/AbstractRational.hpp"
#include "abstract_rational/interval/Interval.hpp"
#include "named/percussion_instrument/PercussionInstrument.hpp"
#include "named/program/instrument/Instrument.hpp"
#include "named/program/percussion_set/PercussionSet.hpp"
#include "other/other.hpp"

static auto json_field_to_words(const nlohmann::json &json_row) -> QString {
  if (json_row.contains("words")) {
    return QString::fromStdString(json_row["words"]);
  }
  return "";
}

auto get_object_schema(const nlohmann::json &properties_json)
    -> nlohmann::json {
  return nlohmann::json({{"type", "object"}, {"properties", properties_json}});
}

Row::Row(const nlohmann::json &json_chord)
    : beats(json_field_to_abstract_rational<Rational>(json_chord, "beats")),
      velocity_ratio(json_field_to_abstract_rational<Rational>(
          json_chord, "velocity_ratio")),
      words(json_field_to_words(json_chord)) {}

[[nodiscard]] auto Row::to_json() const -> nlohmann::json {
  auto json_row = nlohmann::json::object();
  add_abstract_rational_to_json(json_row, beats, "beats");
  add_abstract_rational_to_json(json_row, velocity_ratio, "velocity_ratio");
  add_words_to_json(json_row, words);
  return json_row;
}

auto Row::is_column_editable(int /*column_number*/) -> bool { return true; }

void add_words_to_json(nlohmann::json &json_row, const QString &words) {
  if (!words.isEmpty()) {
    json_row["words"] = words.toStdString().c_str();
  }
}

template <std::derived_from<Named> SubNamed>
static void add_named_schema(nlohmann::json &json_row) {
  std::vector<std::string> names;
  const auto &all_nameds = SubNamed::get_all_nameds();
  std::transform(all_nameds.cbegin(), all_nameds.cend(),
                 std::back_inserter(names),
                 [](const SubNamed &item) { return item.name.toStdString(); });
  json_row[SubNamed::get_field_name()] =
      nlohmann::json({{"type", "string"}, {"enum", std::move(names)}});
};

auto get_rational_fields_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"numerator", get_number_schema("integer", 1, MAX_RATIONAL_NUMERATOR)},
       {"denominator",
        get_number_schema("integer", 1, MAX_RATIONAL_DENOMINATOR)}});
}

void add_pitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<Instrument>(schema);
  auto interval_fields_schema = get_rational_fields_schema();
  interval_fields_schema["octave"] =
      get_number_schema("integer", -MAX_OCTAVE, MAX_OCTAVE);
  schema["interval"] = get_object_schema(interval_fields_schema);
}

void add_unpitched_fields_to_schema(nlohmann::json &schema) {
  add_named_schema<PercussionSet>(schema);
  add_named_schema<PercussionInstrument>(schema);
}

auto Row::get_fields_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"beats", get_object_schema(get_rational_fields_schema())},
       {"velocity_ratio", get_object_schema(get_rational_fields_schema())},
       {"words", nlohmann::json({{"type", "string"}})}});
}

auto json_field_to_interval(const nlohmann::json &json_row) -> Interval {
  return json_field_to_abstract_rational<Interval>(json_row, "interval");
}

void add_interval_to_json(nlohmann::json &json_row, const Interval &interval) {
  add_abstract_rational_to_json(json_row, interval, "interval");
}