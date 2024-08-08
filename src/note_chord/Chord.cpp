#include "note_chord/Chord.hpp"

#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>

#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"
#include "other/templates.hpp"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    const auto &json_notes = json_chord["notes"];
    std::transform(
        json_notes.cbegin(), json_notes.cend(), std::back_inserter(notes),
        [](const nlohmann::json &json_note) { return Note(json_note); });
  }
}

auto Chord::is_chord() const -> bool { return true; }

auto Chord::get_symbol() const -> QString { return "♫"; }

auto Chord::to_json() const -> nlohmann::json {
  auto json_chord = NoteChord::to_json();
  if (!notes.empty()) {
    json_chord["notes"] = items_to_json(notes, 0, notes.size());
  }
  return json_chord;
}

