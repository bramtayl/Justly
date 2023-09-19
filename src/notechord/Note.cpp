#include "notechord/Note.h"

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
#include "notechord/NoteChord.h"  // for error_level, note_level, TreeLevel
#include "utilities.h"

Note::Note() : NoteChord() {}

auto Note::symbol_for() const -> QString { return "♪"; }

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::new_child_pointer() -> std::unique_ptr<NoteChord> {
  error_level(note_level);
  return nullptr;
}

auto Note::verify_json_items(const QString &notes_text) -> bool {
  nlohmann::json parsed_json;
  try {
    parsed_json = nlohmann::json::parse(notes_text.toStdString());
  } catch (const nlohmann::json::parse_error& parse_error) {
    show_parse_error(parse_error);
    return false;
  }

  static const nlohmann::json_schema::json_validator notes_validator(
    nlohmann::json({
      {"$schema", "http://json-schema.org/draft-07/schema#"},
      {"type", "array"},
      {"title", "Notes"},
      {"description", "the notes"},
      {"items", Note::get_schema()}
    })
  );

  JsonErrorHandler error_handler;
  notes_validator.validate(parsed_json, error_handler);
  return !error_handler;
}

auto Note::get_schema() -> const nlohmann::json& {
  static const nlohmann::json note_schema({
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
