#include "note_chord/Note.hpp"

#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "instrument/Instrument.hpp"
#include "note_chord/NoteChord.hpp"
#include "other/templates.hpp"

Note::Note() {
    instrument_pointer = get_instrument_pointer("Marimba");
}

Note::Note(const nlohmann::json &json_note) : NoteChord(json_note) {}

auto notes_to_json(const std::vector<Note> &notes, size_t first_note_number,
                   size_t number_of_notes) -> nlohmann::json {
  nlohmann::json json_notes;
  check_range(notes, first_note_number, number_of_notes);
  std::transform(notes.cbegin() + static_cast<int>(first_note_number),
                 notes.cbegin() +
                     static_cast<int>(first_note_number + number_of_notes),
                 std::back_inserter(json_notes),
                 [](const Note &note) { return note_chord_to_json(&note); });
  return json_notes;
}

void json_to_notes(std::vector<Note> &new_notes,
                   const nlohmann::json &json_notes) {
  std::transform(
      json_notes.cbegin(), json_notes.cend(), std::back_inserter(new_notes),
      [](const nlohmann::json &json_chord) { return Note(json_chord); });
}
