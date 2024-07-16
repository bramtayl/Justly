#include "justly/Chord.hpp"

#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    const auto &json_notes = json_chord["notes"];
    std::transform(
        json_notes.cbegin(), json_notes.cend(),
        std::inserter(notes, notes.begin()),
        [](const nlohmann::json &json_object) { return Note(json_object); });
  }
}

auto Chord::symbol() const -> std::string { return "♫"; }

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!notes.empty()) {
    nlohmann::json json_notes;
    std::transform(notes.cbegin(), notes.cend(), std::back_inserter(json_notes),
                   [](const Note &note) { return note.json(); });
    json_chord["notes"] = json_notes;
  }
  return json_chord;
}

auto get_chord_schema() -> const nlohmann::json & {
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
