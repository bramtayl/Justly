#include "main/Song.h"

#include <qvariant.h>

#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string

#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument
#include "notechord/Chord.h"
#include "utilities/JsonErrorHandler.h"

auto Song::to_json() const -> nlohmann::json {
  nlohmann::json json_object;
  json_object["$schema"] =
      "https://raw.githubusercontent.com/bramtayl/Justly/"
      "master/src/song_schema.json";
  json_object["starting_key"] = starting_key;
  json_object["starting_tempo"] = starting_tempo;
  json_object["starting_volume"] = starting_volume;
  json_object["starting_instrument"] =
      starting_instrument_pointer->instrument_name;
  root.save_to(&json_object);
  return json_object;
}

auto Song::verify_json(const nlohmann::json& parsed_json) -> bool {
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
  return !error_handler;
}

void Song::load_controls(const nlohmann::json& parsed_json) {
  starting_key = parsed_json["starting_key"].get<double>();
  starting_volume = parsed_json["starting_volume"].get<double>();
  starting_tempo = parsed_json["starting_tempo"].get<double>();
  starting_instrument_pointer = &(Instrument::get_instrument_by_name(
      parsed_json["starting_instrument"].get<std::string>()));
}

auto Song::get_starting_value(StartingFieldId value_type) const -> QVariant {
  if (value_type == starting_key_id) {
    return QVariant::fromValue(starting_key);
  }
  if (value_type == starting_volume_id) {
    return QVariant::fromValue(starting_volume);
  }
  if (value_type == starting_tempo_id) {
    return QVariant::fromValue(starting_tempo);
  }
  return QVariant::fromValue(starting_instrument_pointer.get());
}

void Song::set_starting_value(StartingFieldId value_type,
                              const QVariant& new_value) {
  if (value_type == starting_key_id) {
    starting_key = new_value.toInt();
  } else if (value_type == starting_volume_id) {
    starting_volume = new_value.toInt();
  } else if (value_type == starting_tempo_id) {
    starting_tempo = new_value.toInt();
  } else {
    starting_instrument_pointer = new_value.value<const Instrument*>();
  }
}
