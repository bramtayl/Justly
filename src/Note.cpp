#include "Note.h"

#include <qstring.h>        // for QString

#include "JsonErrorHandler.h"
#include "NoteChord.h"  // for error_level, note_level, TreeLevel
#include "utilities.h"

#include <initializer_list>          // for initializer_list
#include <map>                       // for operator!=, operator==
#include <vector>                    // for vector

#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json_fwd.hpp>     // for json

auto Note::get_validator() -> nlohmann::json_schema::json_validator& {
  static nlohmann::json_schema::json_validator validator(nlohmann::json::parse(QString(R"(
  {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "array",
    "title": "Notes",
    "description": "the notes",
    "items": %1
  }
  )").arg(Note::get_schema()).toStdString()));
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
  get_validator().validate(parsed_json, error_handler);
  return !error_handler;
}

auto Note::get_schema() -> QString& {
  static auto note_schema = QString(R"(
{
    "type": "object",
    "description": "a note",
    "properties": {
        %1
    }
})").arg(NoteChord::get_properties_schema());
  return note_schema;
}
