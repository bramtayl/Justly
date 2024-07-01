#include "justly/Chord.hpp"

#include <algorithm>              // for transform
#include <map>                    // for operator!=, operator==
#include <nlohmann/json.hpp>      // for basic_json<>::object_t, basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "json/json.hpp"         // for insert_from_json, objects_to_json
#include "justly/Note.hpp"       // for Note, get_note_schema
#include "justly/NoteChord.hpp"  // for get_note_chord_fields_schema, NoteC...

Chord::Chord(const nlohmann::json& json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    insert_from_json(notes, 0, json_chord["notes"]);
  }
}

auto Chord::symbol() const -> std::string { return "♫"; }

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!notes.empty()) {
    json_chord["notes"] = objects_to_json(notes, 0, notes.size());
  }
  return json_chord;
}

auto get_chord_schema() -> const nlohmann::json& {
  static const nlohmann::json chord_schema = []() {
    auto chord_properties = get_note_chord_fields_schema();
    chord_properties["notes"] = nlohmann::json({{"type", "array"},
                                                {"description", "the notes"},
                                                {"items", get_note_schema()}});
    return nlohmann::json({{"type", "object"},
                           {"description", "a chord"},
                           {"properties", chord_properties}});
  }();
  return chord_schema;
}
