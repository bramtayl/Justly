#include "other/schemas.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <utility>

#include "cell_values/instruments.hpp"
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

auto get_instrument_schema() -> nlohmann::json & {
  static nlohmann::json instrument_schema({{"type", "string"},
                                           {"description", "the instrument"},
                                           {"enum", get_instrument_names()}});
  return instrument_schema;
}

auto get_note_chord_columns_schema() -> const nlohmann::json & {
  static const nlohmann::json note_chord_columns_schema(
      {{"instrument", get_instrument_schema()},
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