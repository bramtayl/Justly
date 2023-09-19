#include "notechord/Chord.h"

#include <qstring.h>  // for QString

#include <map>               // for operator!=, operator==
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <initializer_list>              // for initializer_list
#include <vector>                        // for vector

#include "metatypes/Interval.h"                    // for Interval
#include "JsonErrorHandler.h"
#include "notechord/Note.h"       // for Note
#include "notechord/NoteChord.h"  // for NoteChord, TreeLevel, chord_level
#include "utilities.h"  // for verify_json_array, verify_json_object

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::verify_json_items(const QString &chords_text) -> bool {
  nlohmann::json parsed_json;
  try {
    parsed_json = nlohmann::json::parse(chords_text.toStdString());
  } catch (const nlohmann::json::parse_error& parse_error) {
    show_parse_error(parse_error);
    return false;
  }

  static const nlohmann::json_schema::json_validator chords_validator(
    nlohmann::json({
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"title", "Chords"},
      {"description", "a list of chords"},
      {"type", "array"},
      {"items", Chord::get_schema()},
    })
  );

  JsonErrorHandler error_handler;
  chords_validator.validate(parsed_json, error_handler);
  return !error_handler;
}

auto Chord::get_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema({
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