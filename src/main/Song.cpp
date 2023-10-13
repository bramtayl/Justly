#include "main/Song.h"

#include <qvariant.h>

#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <utility>

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
  nlohmann::json chords_array;
  for (const auto &chord_pointer : chord_pointers) {
    nlohmann::json chord_object = nlohmann::json::object();
    chord_pointer->save_to(&chord_object);
    chords_array.push_back(std::move(chord_object));
  }
  json_object["chords"] = std::move(chords_array);
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

void Song::load_from(const nlohmann::json& parsed_json) {
  starting_key = parsed_json["starting_key"].get<double>();
  starting_volume = parsed_json["starting_volume"].get<double>();
  starting_tempo = parsed_json["starting_tempo"].get<double>();
  starting_instrument_pointer = &(Instrument::get_instrument_by_name(
      parsed_json["starting_instrument"].get<std::string>()));
  remove_chords(0, static_cast<int>(chord_pointers.size()));
  if (parsed_json.contains("chords")) {
    insert_json_chords(0, parsed_json["chords"]);
  }
}

auto Song::get_starting_value(StartingFieldId value_type) const -> QVariant {
  switch (value_type) {
    case starting_key_id:
      return QVariant::fromValue(starting_key);
    case starting_volume_id:
      return QVariant::fromValue(starting_volume);
    case starting_tempo_id:
      return QVariant::fromValue(starting_tempo);
    default:  // starting_instrument_id
      return QVariant::fromValue(starting_instrument_pointer.get());
  }
}

void Song::set_starting_value(StartingFieldId value_type,
                              const QVariant& new_value) {
  switch (value_type) {
    case starting_key_id:
      starting_key = new_value.toInt();
      break;
    case starting_volume_id:
      starting_volume = new_value.toInt();
      break;
    case starting_tempo_id:
      starting_tempo = new_value.toInt();
      break;
    case starting_instrument_id:
      starting_instrument_pointer = new_value.value<const Instrument*>();
  }
}

auto Song::copy_json_chords(int first_chord_number,
                                  int number_of_chords) const
    -> nlohmann::json {
  nlohmann::json json_children;
  for (int index = first_chord_number;
       index < first_chord_number + number_of_chords; index = index + 1) {
    nlohmann::json json_child = nlohmann::json::object();
    chord_pointers[index]->save_to(&json_child);
    json_children.push_back(std::move(json_child));
  }
  return json_children;
}

void Song::insert_empty_chords(int first_chord_number, int number_of_chords) {
  for (int index = first_chord_number;
       index < first_chord_number + number_of_chords; index = index + 1) {
    // will error if childless
    chord_pointers.insert(chord_pointers.begin() + index,
                          std::make_unique<Chord>());
  }
}

void Song::remove_chords(int first_chord_number, int number_of_chords) {
  chord_pointers.erase(
      chord_pointers.begin() + first_chord_number,
      chord_pointers.begin() + first_chord_number + number_of_chords);
}

void Song::insert_json_chords(int first_chord_number,
                                    const nlohmann::json &insertion) {
  for (int offset = 0; offset < static_cast<int>(insertion.size());
       offset = offset + 1) {
    const auto &chord_object = insertion[offset];
    auto new_chord_pointer = std::make_unique<Chord>();
    new_chord_pointer->load_from(chord_object);
    chord_pointers.insert(chord_pointers.begin() + first_chord_number + offset,
                          std::move(new_chord_pointer));
  }
}

