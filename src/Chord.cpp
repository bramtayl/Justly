#include "Chord.h"

#include <qstring.h>        // for QString

#include "JsonErrorHandler.h"
#include "Note.h"           // for Note
#include "NoteChord.h"  // for NoteChord, TreeLevel, chord_level
#include "utilities.h"      // for verify_json_array, verify_json_object

#include <initializer_list>          // for initializer_list
#include <map>                       // for operator!=, operator==
#include <vector>                    // for vector

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json_fwd.hpp>     // for json

auto Chord::get_validator() -> nlohmann::json_schema::json_validator& {

  static nlohmann::json_schema::json_validator validator(nlohmann::json::parse(QString(R"(
  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "title": "Chords",
    "type": "array",
    "description": "a list of chords",
    "items": %1
  }
  )").arg(Chord::get_schema()).toStdString()));
  return validator;
}

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::verify_json_items(const QString& chord_text) -> bool {
  nlohmann::json parsed_json;
  if (!(parse_json(parsed_json, chord_text))) {
    return false;
  }

  JsonErrorHandler error_handler;
  get_validator().validate(parsed_json, error_handler);
  return !error_handler;
}

auto Chord::get_schema() -> QString& {
  static auto chord_schema = QString(R"(
  {
      "type": "object",
      "description": "a chord",
      "properties": {
          %1,
          "notes": {
              "type": "array",
              "description": "the notes",
              "items": %2
          }
      }
  }
  )")
  .arg(NoteChord::get_properties_schema())
  .arg(Note::get_schema());
  return chord_schema;
}