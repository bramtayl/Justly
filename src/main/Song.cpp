#include "main/Song.h"

#include <qbytearray.h>  // for QByteArray
#include <qstring.h>     // for QString

#include <initializer_list>                  // for initializer_list
#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>

#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument
#include "notechord/Chord.h"
#include "utilities/JsonErrorHandler.h"
#include "utilities/utilities.h"  // for require_json_field, parse_error

auto Song::to_json() const -> nlohmann::json {
  nlohmann::json json_object;
  json_object["$schema"] =
      "https://raw.githubusercontent.com/bramtayl/Justly/"
      "master/src/song_schema.json";
  json_object["starting_key"] = starting_key;
  json_object["starting_tempo"] = starting_tempo;
  json_object["starting_volume"] = starting_volume;
  json_object["starting_instrument"] =
      get_starting_instrument().instrument_name.toStdString();
  root.save_to(json_object);
  return json_object;
}

auto Song::load_text(const QByteArray& song_text) -> bool {
  nlohmann::json parsed_json;
  try {
    parsed_json = nlohmann::json::parse(song_text.toStdString());
  } catch (const nlohmann::json::parse_error& parse_error) {
    show_parse_error(parse_error);
    return false;
  }

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
                          {"enum", Instrument::get_all_instrument_names()}}},
                        {"starting_key",
                         {{"type", "integer"},
                          {"description", "the starting key, in Hz"},
                          {"minimum", MINIMUM_STARTING_KEY},
                          {"maximum", MAXIMUM_STARTING_KEY}}},
                        {"starting_tempo",
                         {{"type", "integer"},
                          {"description", "the starting tempo, in bpm"},
                          {"minimum", MINIMUM_STARTING_TEMPO},
                          {"maximum", MAXIMUM_STARTING_TEMPO}}},
                        {"starting_volume",
                         {{"type", "integer"},
                          {"description", "the starting volume, from 1 to 100"},
                          {"minimum", MINIMUM_STARTING_VOLUME},
                          {"maximum", MAXIMUM_STARTING_VOLUME}}},
                        {"chords",
                         {{"type", "array"},
                          {"description", "a list of chords"},
                          {"items", Chord::get_schema()}}}}}}));

  validator.validate(parsed_json, error_handler);

  if (error_handler) {
    return false;
  }

  starting_key = parsed_json["starting_key"].get<double>();
  starting_volume = parsed_json["starting_volume"].get<double>();
  starting_tempo = parsed_json["starting_tempo"].get<double>();
  set_starting_instrument(
      Instrument::get_instrument_by_name(QString::fromStdString(
          parsed_json["starting_instrument"].get<std::string>())));

  root.load_from(parsed_json);
  return true;
}

auto Song::get_starting_instrument() const -> const Instrument& {
  return *starting_instrument_pointer;
}

void Song::set_starting_instrument(const Instrument& new_instrument) {
  starting_instrument_pointer = &new_instrument;
};