#include "Chord.h"

#include <qstring.h>  // for QString

#include <map>               // for operator!=, operator==
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json

#include "Interval.h"                    // for Interval
#include "JsonErrorHandler.h"
#include "Note.h"       // for Note
#include "NoteChord.h"  // for NoteChord, TreeLevel, chord_level
#include "utilities.h"  // for verify_json_array, verify_json_object

auto Chord::get_list_validator() -> nlohmann::json_schema::json_validator & {
  static nlohmann::json_schema::json_validator validator(
    nlohmann::json({
      {"title", "Chords"},
      {"description", "a list of chords"},
      {"type", "array"},
      {"items", Chord::get_schema()},
    })
  );
  return validator;
}

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::verify_json_items(const QString &chord_text) -> bool {
  nlohmann::json parsed_json;
  if (!(parse_json(parsed_json, chord_text))) {
    return false;
  }

  JsonErrorHandler error_handler;
  get_list_validator().validate(parsed_json, error_handler);
  return !error_handler;
}

auto Chord::get_schema() -> nlohmann::json & {
  static nlohmann::json chord_schema({
    {"type", "object"},
    {"description", "a chord"},
    {"properties", {
      {"interval", Interval::get_schema()},
      {"tempo_percent", NoteChord::get_tempo_percent_schema()},
      {"volume_percent", NoteChord::get_volume_percent_schema()},
      {"beats", NoteChord::get_beats_schema()},
      {"words", NoteChord::get_words_schema()},
      {"instrument", NoteChord::get_instrument_schema()},
      {"notes", {
        {"type", "array"},
        {"description", "the notes"},
        {"items", Note::get_schema()}
      }}
    }}
  });
  return chord_schema;
}