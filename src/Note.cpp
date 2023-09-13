#include "Note.h"

#include <qstring.h>  // for QString

#include <map>               // for operator!=, operator==
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json

#include "Interval.h"                    // for Interval
#include "JsonErrorHandler.h"
#include "NoteChord.h"  // for error_level, note_level, TreeLevel
#include "utilities.h"

auto Note::get_list_validator() -> nlohmann::json_schema::json_validator & {
  static nlohmann::json_schema::json_validator validator(
    nlohmann::json({
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"type", "array"},
      {"title", "Notes"},
      {"description", "the notes"},
      {"items", Note::get_schema()}
    })
  );
  return validator;
}

Note::Note() : NoteChord() {}

auto Note::symbol_for() const -> QString { return "♪"; }

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::new_child_pointer() -> std::unique_ptr<NoteChord> {
  error_level(note_level);
  return nullptr;
}

auto Note::verify_json_items(const QString &note_text) -> bool {
  nlohmann::json parsed_json;
  if (!(parse_json(parsed_json, note_text))) {
    return false;
  }

  JsonErrorHandler error_handler;
  get_list_validator().validate(parsed_json, error_handler);
  return !error_handler;
}

auto Note::get_schema() -> nlohmann::json& {
  static nlohmann::json note_schema({
    {"type", "object"},
    {"description", "a note"},
    {"properties", {
      {"interval", Interval::get_schema()},
      {"tempo_percent", NoteChord::get_tempo_percent_schema()},
      {"volume_percent", NoteChord::get_volume_percent_schema()},
      {"beats", NoteChord::get_beats_schema()},
      {"words", NoteChord::get_words_schema()},
      {"instrument", NoteChord::get_instrument_schema()}
    }}
  });
  return note_schema;
}
