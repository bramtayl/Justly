#include "justly/Note.hpp"

#include <nlohmann/json.hpp>

#include "justly/NoteChord.hpp"

Note::Note(const nlohmann::json &json_note) : NoteChord(json_note) {}

auto get_notes_schema() -> const nlohmann::json & {
  static const nlohmann::json notes_schema(
      {{"type", "array"},
       {"description", "the notes"},
       {"items",
        {{"type", "object"},
         {"description", "a note"},
         {"properties", get_note_chord_columns_schema()}}}});
  return notes_schema;
}
