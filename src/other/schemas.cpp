#include "other/schemas.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <utility>

#include "cell_values/instruments.hpp"
#include "justly/Instrument.hpp"
#include "other/private_constants.hpp"

auto get_rational_schema(const std::string &description) -> nlohmann::json {
  return nlohmann::json({{"type", "object"},
                         {"description", description},
                         {"properties",
                          {{"numerator",
                            {{"type", "integer"},
                             {"description", "the numerator"},
                             {"minimum", 1},
                             {"maximum", MAX_RATIONAL_NUMERATOR}}},
                           {"denominator",
                            {{"type", "integer"},
                             {"description", "the denominator"},
                             {"minimum", 1},
                             {"maximum", MAX_RATIONAL_DENOMINATOR}}}}}});
}

auto get_instrument_schema(const std::string &description) -> nlohmann::json {
  static const std::vector<std::string> instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto &all_instruments = get_all_instruments();
    std::transform(all_instruments.cbegin(), all_instruments.cend(),
                   std::back_inserter(temp_names),
                   [](const Instrument &instrument) {
                     return instrument.instrument_name;
                   });
    return temp_names;
  }();
  return nlohmann::json({{"type", "string"},
                         {"description", description},
                         {"enum", instrument_names}});
}

auto get_note_chord_columns_schema() -> const nlohmann::json & {
  static const nlohmann::json note_chord_columns_schema(
      {{"instrument", get_instrument_schema("the instrument")},
       {"interval",
        {{"type", "object"},
         {"description", "an interval"},
         {"properties",
          {{"numerator",
            {{"type", "integer"},
             {"description", "the numerator"},
             {"minimum", 1},
             {"maximum", MAX_INTERVAL_NUMERATOR}}},
           {"denominator",
            {{"type", "integer"},
             {"description", "the denominator"},
             {"minimum", 1},
             {"maximum", MAX_INTERVAL_DENOMINATOR}}},
           {"octave",
            {{"type", "integer"},
             {"description", "the octave"},
             {"minimum", MIN_OCTAVE},
             {"maximum", MAX_OCTAVE}}}}}}},
       {"beats", get_rational_schema("the number of beats")},
       {"volume_percent", get_rational_schema("volume ratio")},
       {"tempo_percent", get_rational_schema("tempo ratio")},
       {"words", {{"type", "string"}, {"description", "the words"}}}});
  return note_chord_columns_schema;
}

auto get_notes_schema() -> const nlohmann::json & {
  static const nlohmann::json notes_schema(
      {{"type", "array"},
       {"description", "the notes"},
       {"items",
        {{"type", "object"},
         {"description", "a note"},
         {"properties", get_note_chord_columns_schema()}}}});
  return notes_schema;
}

auto get_chords_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema = []() {
    auto chord_properties = get_note_chord_columns_schema();
    chord_properties["notes"] = get_notes_schema();
    return nlohmann::json({{"type", "array"},
                           {"description", "a list of chords"},
                           {"items",
                            {{"type", "object"},
                             {"description", "a chord"},
                             {"properties", std::move(chord_properties)}}}});
  }();
  return chord_schema;
}

auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator {
  json["$schema"] = "http://json-schema.org/draft-07/schema#";
  json["title"] = title;
  return {json};
}