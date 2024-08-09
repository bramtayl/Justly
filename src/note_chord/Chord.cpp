#include "note_chord/Chord.hpp"

#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>

#include "note_chord/NoteChord.hpp"
#include "other/templates.hpp"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    const auto &json_notes = json_chord["notes"];
    json_to_notes(notes, json_notes);
  }
}

auto Chord::is_chord() const -> bool { return true; }

auto chords_to_json(const std::vector<Chord> &chords, size_t first_chord_number,
                    size_t number_of_chords) -> nlohmann::json {
  nlohmann::json json_chords;
  check_range(chords, first_chord_number, number_of_chords);
  std::transform(
      chords.cbegin() + static_cast<int>(first_chord_number),
      chords.cbegin() + static_cast<int>(first_chord_number + number_of_chords),
      std::back_inserter(json_chords), [](const Chord &chord) {
        auto json_chord = note_chord_to_json(&chord);
        const auto& notes = chord.notes;
        if (!notes.empty()) {
          json_chord["notes"] = notes_to_json(notes, 0, notes.size());
        }
        return json_chord;
      });
  return json_chords;
}

void json_to_chords(std::vector<Chord> &new_chords,
                    const nlohmann::json &json_chords) {
  std::transform(
      json_chords.cbegin(), json_chords.cend(), std::back_inserter(new_chords),
      [](const nlohmann::json &json_chord) { return Chord(json_chord); });
}
