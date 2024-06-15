#include "schemas.h"

#include <algorithm>                     // for transform
#include <iterator>                      // for back_insert_iterator, back_i...
#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>      // for json_validator
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, basi...
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>                        // for string, basic_string
#include <vector>                        // for vector

#include "justly/Instrument.h"     // for get_all_instruments, Instrument
#include "justly/Interval.h"       // for MAXIMUM_DENOMINATOR, MAXIMUM...
#include "justly/NoteChord.h"      // for MAXIMUM_BEATS, MAXIMUM_TEMPO...
#include "justly/Song.h"           // for MAX_STARTING_KEY, MAX_STARTI...
#include "src/JsonErrorHandler.h"  // for JsonErrorHandler

auto instrument_names() -> const std::vector<std::string>& {
  static const std::vector<std::string> all_instrument_names = []() {
    std::vector<std::string> temp_names;
    const auto& all_instruments = get_all_instruments();
    std::transform(all_instruments.cbegin(), all_instruments.cend(),
                   std::back_inserter(temp_names),
                   [](const Instrument& instrument) {
                     return instrument.instrument_name;
                   });
    return temp_names;
  }();
  return all_instrument_names;
}

auto instrument_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema({{"type", "string"},
                                           {"description", "the instrument"},
                                           {"enum", instrument_names()}});
  return instrument_schema;
}

auto beats_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema(
      {{"type", "integer"},
       {"description", "the number of beats"},
       {"minimum", MINIMUM_BEATS},
       {"maximum", MAXIMUM_BEATS}});
  return instrument_schema;
}

auto words_schema() -> nlohmann::json& {
  static nlohmann::json words_schema(
      {{"type", "string"}, {"description", "the words"}});
  return words_schema;
}

auto volume_percent_schema() -> nlohmann::json& {
  static nlohmann::json volume_percent_schema(
      {{"type", "number"},
       {"description", "the volume percent"},
       {"minimum", MINIMUM_VOLUME_PERCENT},
       {"maximum", MAXIMUM_VOLUME_PERCENT}});
  return volume_percent_schema;
}

auto tempo_percent_schema() -> nlohmann::json& {
  static nlohmann::json tempo_percent_schema(
      {{"type", "number"},
       {"description", "the tempo percent"},
       {"minimum", MINIMUM_TEMPO_PERCENT},
       {"maximum", MAXIMUM_TEMPO_PERCENT}});
  return tempo_percent_schema;
}

auto interval_json_schema() -> const nlohmann::json& {
  static const nlohmann::json interval_schema(
      {{"type", "object"},
       {"description", "an interval"},
       {"properties",
        {{"numerator",
          {{"type", "integer"},
           {"description", "the numerator"},
           {"minimum", MINIMUM_NUMERATOR},
           {"maximum", MAXIMUM_NUMERATOR}}},
         {"denominator",
          {{"type", "integer"},
           {"description", "the denominator"},
           {"minimum", MINIMUM_DENOMINATOR},
           {"maximum", MAXIMUM_DENOMINATOR}}},
         {"octave",
          {{"type", "integer"},
           {"description", "the octave"},
           {"minimum", MINIMUM_OCTAVE},
           {"maximum", MAXIMUM_OCTAVE}}}}}});
  return interval_schema;
}

auto note_schema() -> const nlohmann::json& {
  static const nlohmann::json note_schema(
      {{"type", "object"},
       {"description", "a note"},
       {"properties",
        {{"interval", interval_json_schema()},
         {"tempo_percent", tempo_percent_schema()},
         {"volume_percent", volume_percent_schema()},
         {"beats", beats_schema()},
         {"words", words_schema()},
         {"instrument", instrument_schema()}}}});
  return note_schema;
}

auto chord_schema() -> const nlohmann::json& {
  static const nlohmann::json chord_schema(
      {{"type", "object"},
       {"description", "a chord"},
       {"properties",
        {{"interval", interval_json_schema()},
         {"tempo_percent", tempo_percent_schema()},
         {"volume_percent", volume_percent_schema()},
         {"beats", beats_schema()},
         {"words", words_schema()},
         {"instrument", instrument_schema()},
         {"notes",
          {{"type", "array"},
           {"description", "the notes"},
           {"items", note_schema()}}}}}});
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
                          {"enum", instrument_names()}}},
                        {"starting_key",
                         {{"type", "integer"},
                          {"description", "the starting key, in Hz"},
                          {"minimum", MIN_STARTING_KEY},
                          {"maximum", MAX_STARTING_KEY}}},
                        {"starting_tempo",
                         {{"type", "integer"},
                          {"description", "the starting tempo, in bpm"},
                          {"minimum", MIN_STARTING_TEMPO},
                          {"maximum", MAX_STARTING_TEMPO}}},
                        {"starting_volume",
                         {{"type", "integer"},
                          {"description", "the starting volume, from 1 to 100"},
                          {"minimum", MIN_STARTING_VOLUME},
                          {"maximum", MAX_STARTING_VOLUME}}},
                        {"chords",
                         {{"type", "array"},
                          {"description", "a list of chords"},
                          {"items", chord_schema()}}}}}}));
  validator.validate(json_song, error_handler);
  return !error_handler;
}