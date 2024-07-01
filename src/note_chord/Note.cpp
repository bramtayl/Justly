#include "justly/Note.hpp"

#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, basi...
#include <string>                        // for allocator, string

#include "justly/NoteChord.hpp"  // for get_note_chord_fields_schema

Note::Note(const nlohmann::json& json_note) : NoteChord(json_note) {}

auto Note::symbol() const -> std::string { return "♪"; }

auto get_note_schema() -> const nlohmann::json& {
  static const nlohmann::json note_schema(
      {{"type", "object"},
       {"description", "a note"},
       {"properties", get_note_chord_fields_schema()}});
  return note_schema;
}
