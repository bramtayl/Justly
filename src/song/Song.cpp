#include "justly/Song.hpp"

#include <qassert.h>  // for Q_ASSERT

#include <algorithm>                         // for transform
#include <cstddef>                           // for size_t
#include <map>                               // for operator!=, operator==
#include <memory>                            // for allocator_traits<>::valu...
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json-schema.hpp>          // for json_validator
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string

#include "json/JsonErrorHandler.hpp"
#include "json/json.hpp"          // for insert_from_json, objects_to_json
#include "justly/Chord.hpp"       // for Chord
#include "justly/Instrument.hpp"  // for get_instrument_pointer
#include "justly/Note.hpp"        // for get_note_schema
#include "justly/NoteChordField.hpp"
#include "other/private_constants.hpp"

struct NoteChord;

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 100;
const auto DEFAULT_STARTING_INSTRUMENT = "Marimba";

Song::Song()
    : starting_key(DEFAULT_STARTING_KEY),
      starting_volume(DEFAULT_STARTING_VOLUME),
      starting_tempo(DEFAULT_STARTING_TEMPO),
      starting_instrument_pointer(
          get_instrument_pointer(DEFAULT_STARTING_INSTRUMENT)) {}

auto Song::get_number_of_children(int parent_number) const -> size_t {
  auto chords_size = chords.size();
  if (parent_number == -1) {
    return chords_size;
  }

  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  const auto& chord = chords[parent_number];

  return chord.notes.size();
};

auto Song::get_chord_number(Chord* chord_pointer) const -> int {
  for (size_t chord_number = 0; chord_number < chords.size();
       chord_number = chord_number + 1) {
    if (chord_pointer == &chords[chord_number]) {
      return static_cast<int>(chord_number);
    }
  }
  Q_ASSERT(false);
  return 0;
}

[[nodiscard]] auto Song::get_note_chord_pointer(int parent_number,
                                                size_t child_number)
    -> NoteChord* {
  if (parent_number == -1) {
    Q_ASSERT(child_number < chords.size());
    return &chords[child_number];
  }
  Q_ASSERT(parent_number >= 0);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  auto& chord = chords[parent_number];

  auto& notes = chord.notes;
  Q_ASSERT(child_number < notes.size());
  return &notes[child_number];
}

[[nodiscard]] auto Song::get_const_note_chord_pointer(int parent_number,
                                                      size_t child_number) const
    -> const NoteChord* {
  if (parent_number == -1) {
    Q_ASSERT(child_number < chords.size());
    return &chords[child_number];
  }
  Q_ASSERT(parent_number >= 0);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  const auto& chord = chords[parent_number];

  const auto& notes = chord.notes;
  Q_ASSERT(child_number < notes.size());
  return &notes[child_number];
}

auto Song::json() const -> nlohmann::json {
  nlohmann::json json_song;
  json_song["starting_key"] = starting_key;
  json_song["starting_tempo"] = starting_tempo;
  json_song["starting_volume"] = starting_volume;
  Q_ASSERT(starting_instrument_pointer != nullptr);
  json_song["starting_instrument"] =
      starting_instrument_pointer->instrument_name;
  json_song["chords"] = objects_to_json(chords, 0, chords.size());
  return json_song;
}

void Song::load_starting_values(const nlohmann::json& json_song) {
  Q_ASSERT(json_song.contains("starting_key"));
  const auto& starting_key_value = json_song["starting_key"];
  Q_ASSERT(starting_key_value.is_number());
  starting_key = starting_key_value.get<double>();

  Q_ASSERT(json_song.contains("starting_volume"));
  const auto& starting_volume_value = json_song["starting_volume"];
  Q_ASSERT(starting_volume_value.is_number());
  starting_volume = starting_volume_value.get<double>();

  Q_ASSERT(json_song.contains("starting_tempo"));
  const auto& starting_tempo_value = json_song["starting_tempo"];
  Q_ASSERT(starting_tempo_value.is_number());
  starting_tempo = starting_tempo_value.get<double>();

  Q_ASSERT(json_song.contains("starting_instrument"));
  const auto& starting_instrument_value = json_song["starting_instrument"];
  Q_ASSERT(starting_instrument_value.is_string());
  starting_instrument_pointer =
      get_instrument_pointer(starting_instrument_value.get<std::string>());
}

void Song::load_chords(const nlohmann::json& json_song) {
  chords.clear();
  if (json_song.contains("chords")) {
    insert_from_json(chords, 0, json_song["chords"]);
  }
}

auto Song::copy_cell(CellIndex cell_index) const -> nlohmann::json {
  const auto* note_chord_pointer = get_const_note_chord_pointer(
      cell_index.parent_number, cell_index.child_number);
  switch (cell_index.note_chord_field) {
    case symbol_column: {
      Q_ASSERT(false);
      return {};
    };
    case instrument_column: {
      return note_chord_pointer->instrument_pointer->instrument_name;
    }
    case interval_column: {
      return note_chord_pointer->interval.json();
    };
    case beats_column: {
      return note_chord_pointer->beats.json();
    };
    case volume_ratio_column: {
      return note_chord_pointer->volume_ratio.json();
    };
    case tempo_ratio_column: {
      return note_chord_pointer->tempo_ratio.json();
    };
    case words_column: {
      return note_chord_pointer->words;
    };
  }
}

auto Song::copy_rows(size_t first_child_number, size_t number_of_children,
                     int parent_number) const -> nlohmann::json {
  if (parent_number == -1) {
    // or root
    return objects_to_json(chords, first_child_number, number_of_children);
  }
  // for chord
  Q_ASSERT(0 <= parent_number);
  Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
  return objects_to_json(chords[parent_number].notes, first_child_number,
                         number_of_children);
}

void Song::insert_directly(size_t first_child_number,
                           const nlohmann::json& json_children,
                           int parent_number) {
  if (parent_number == -1) {
    insert_from_json(chords, first_child_number, json_children);
  } else {
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) < chords.size());
    // for a chord
    insert_from_json(chords[parent_number].notes, first_child_number,
                     json_children);
  }
}

void Song::remove_directly(size_t first_child_number, size_t number_of_children,
                           int parent_number) {
  auto chords_size = chords.size();

  auto int_first_child_number = static_cast<int>(first_child_number);

  auto end_number = first_child_number + number_of_children;
  auto int_end_number = static_cast<int>(end_number);

  if (parent_number == -1) {
    // for root
    Q_ASSERT(first_child_number < chords.size());
    Q_ASSERT(end_number <= chords_size);
    chords.erase(chords.begin() + int_first_child_number,
                 chords.begin() + int_end_number);
  } else {
    // for a chord
    Q_ASSERT(0 <= parent_number);
    Q_ASSERT(static_cast<size_t>(parent_number) < chords_size);
    auto& chord = chords[parent_number];

    auto& notes = chord.notes;
    auto notes_size = notes.size();

    Q_ASSERT(first_child_number < notes_size);

    Q_ASSERT(0 < end_number);
    Q_ASSERT(end_number <= notes_size);

    notes.erase(notes.begin() + int_first_child_number,
                notes.begin() + int_end_number);
  }
}

auto verify_json_cell(NoteChordField note_chord_field,
                     const nlohmann::json& json_children) -> bool {
}

auto verify_children(TreeLevel parent_level,
                     const nlohmann::json& json_children) -> bool {
  static const nlohmann::json_schema::json_validator chords_validator(
      nlohmann::json({
          {"$schema", "http://json-schema.org/draft-07/schema#"},
          {"title", "Chords"},
          {"description", "a list of chords"},
          {"type", "array"},
          {"items", get_chord_schema()},
      }));
  static const nlohmann::json_schema::json_validator notes_validator(
      nlohmann::json({{"$schema", "http://json-schema.org/draft-07/schema#"},
                      {"type", "array"},
                      {"title", "Notes"},
                      {"description", "the notes"},
                      {"items", get_note_schema()}}));
  JsonErrorHandler error_handler;
  if (parent_level == root_level) {
    chords_validator.validate(json_children, error_handler);
  } else {
    Q_ASSERT(parent_level == chord_level);
    notes_validator.validate(json_children, error_handler);
  }
  return !error_handler;
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