#include "schemas.hpp"

#include <algorithm>                     // for transform
#include <iterator>                      // for back_insert_iterator, back_i...
#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, basi...
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>                        // for string, basic_string
#include <vector>                        // for vector

#include "json/JsonErrorHandler.hpp"  // for JsonErrorHandler
#include "justly/Instrument.hpp"      // for get_all_instruments, Instrument
#include "justly/Interval.hpp"        // for MAX_DENOMINATOR, MAXIMUM...
#include "justly/NoteChord.hpp"       // for MAX_BEATS, MAXIMUM_TEMPO...
#include "justly/Song.hpp"            // for MAX_STARTING_KEY, MAX_STARTI...
#include "song/instruments.hpp"
#include "song/private_constants.hpp"

auto get_instrument_names() -> const std::vector<std::string>& {
  static const std::vector<std::string> instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto& all_instruments = get_all_instruments();
    std::transform(all_instruments.cbegin(), all_instruments.cend(),
                   std::back_inserter(temp_names),
                   [](const Instrument& instrument) {
                     return instrument.instrument_name;
                   });
    return temp_names;
  }();
  return instrument_names;
}

auto get_instrument_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema({{"type", "string"},
                                           {"description", "the instrument"},
                                           {"enum", get_instrument_names()}});
  return instrument_schema;
}

auto get_interval_schema() -> const nlohmann::json& {
  static const nlohmann::json interval_schema(
      {{"type", "object"},
       {"description", "an interval"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", 1},
           {"maximum", MAX_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", 1},
           {"maximum", MAX_DENOMINATOR}}},
         {"octave",
          {{"type", "integer"},
           {"description", "the octave"},
           {"minimum", MIN_OCTAVE},
           {"maximum", MAX_OCTAVE}}}}}});
  return interval_schema;
}

auto get_beats_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema(
      {{"type", "object"},
       {"description", "the number of beats"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", 1},
           {"maximum", MAX_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", 1},
           {"maximum", MAX_DENOMINATOR}}}}}});
  return instrument_schema;
}

auto get_volume_ratio_schema() -> const nlohmann::json& {
  static const nlohmann::json volume_ratio_schema(
      {{"type", "object"},
       {"description", "volume ratio"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", 1},
           {"maximum", MAX_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", 1},
           {"maximum", MAX_DENOMINATOR}}}}}});
  return volume_ratio_schema;
}

auto get_tempo_ratio_schema() -> const nlohmann::json& {
  static const nlohmann::json tempo_ratio_schema(
      {{"type", "object"},
       {"description", "tempo ratio"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", 1},
           {"maximum", MAX_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", 1},
           {"maximum", MAX_DENOMINATOR}}}}}});
  return tempo_ratio_schema;
}

auto get_words_schema() -> nlohmann::json& {
  static nlohmann::json words_schema(
      {{"type", "string"}, {"description", "the words"}});
  return words_schema;
}

auto get_note_schema() -> const nlohmann::json& {
  static const nlohmann::json note_schema(
      {{"type", "object"},
       {"description", "a note"},
       {"properties",
        {{"instrument", get_instrument_schema()},
         {"interval", get_interval_schema()},
         {"beats", get_beats_schema()},
         {"volume_percent", get_volume_ratio_schema()},
         {"tempo_percent", get_tempo_ratio_schema()},
         {"words", get_words_schema()}}}});
  return note_schema;
}

auto get_chord_schema() -> const nlohmann::json& {
  static const nlohmann::json chord_schema(
      {{"type", "object"},
       {"description", "a chord"},
       {"properties",
        {{"instrument", get_instrument_schema()},
         {"interval", get_interval_schema()},
         {"beats", get_beats_schema()},
         {"volume_percent", get_volume_ratio_schema()},
         {"tempo_percent", get_tempo_ratio_schema()},
         {"words", get_words_schema()},
         {"notes",
          {{"type", "array"},
           {"description", "the notes"},
           {"items", get_note_schema()}}}}}});
  return chord_schema;
}

auto verify_json_song(const nlohmann::json& json_song) -> bool {
  JsonErrorHandler error_handler;
  static const nlohmann::json_schema::json_validator validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"title", "Song"},
                      {"description", "A Justly song in JSON format"},
                      {"type", "object"},
                      {"required",
                       {"starting_key", "starting_tempo", "starting_volume",
                        "starting_instrument"}},
                      {"properties",
                       {{"starting_instrument",
                         {{"type", "string"},
                          {"description", "the starting instrument"},
                          {"enum", get_instrument_names()}}},
                        {"starting_key",
                         {{"type", "number"},
                          {"description", "the starting key, in Hz"},
                          {"minimum", MIN_STARTING_KEY},
                          {"maximum", MAX_STARTING_KEY}}},
                        {"starting_tempo",
                         {{"type", "number"},
                          {"description", "the starting tempo, in bpm"},
                          {"minimum", MIN_STARTING_TEMPO},
                          {"maximum", MAX_STARTING_TEMPO}}},
                        {"starting_volume",
                         {{"type", "number"},
                          {"description", "the starting volume, from 1 to 100"},
                          {"minimum", 1},
                          {"maximum", MAX_STARTING_VOLUME}}},
                        {"chords",
                         {{"type", "array"},
                          {"description", "a list of chords"},
                          {"items", get_chord_schema()}}}}}}));
  validator.validate(json_song, error_handler);
  return !error_handler;
}