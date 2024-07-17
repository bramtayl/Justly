#include "justly/Note.hpp"

#include <nlohmann/json.hpp>

#include "justly/NoteChord.hpp"

Note::Note(const nlohmann::json &json_note) : NoteChord(json_note) {}

auto Note::symbol() const -> QString { return "♪"; }

auto get_note_schema() -> const nlohmann::json & {
  static const nlohmann::json note_schema(
      {{"type", "object"},
       {"description", "a note"},
       {"properties", get_note_chord_fields_schema()}});
  return note_schema;
}
