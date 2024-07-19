#include "justly/Chord.hpp"

#include <QtGlobal>
#include <algorithm>
#include <iterator>
// IWYU pragma: no_include <map>
#include <nlohmann/json.hpp>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    insert_json_notes(0, json_chord["notes"]);
  }
}

auto Chord::symbol() const -> QString { return "♫"; }

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!notes.empty()) {
    json_chord["notes"] = copy_notes_to_json(0, notes.size());
  }
  return json_chord;
}

void Chord::check_note_number(size_t note_number) const {
  Q_ASSERT(note_number < notes.size());
}

void Chord::check_new_note_number(size_t note_number) const {
  Q_ASSERT(note_number <= notes.size());
}

auto Chord::get_const_note(size_t note_number) const -> const Note & {
  check_note_number(note_number);
  return notes[note_number];
}

auto Chord::get_note(size_t note_number) -> Note & {
  check_note_number(note_number);
  return notes[note_number];
}

auto Chord::copy_notes(size_t first_note_number,
                       size_t number_of_notes) const -> std::vector<Note> {
  auto end_number = first_note_number + number_of_notes;
  check_note_number(first_note_number);
  check_new_note_number(end_number);
  return {notes.cbegin() + static_cast<int>(first_note_number),
          notes.cbegin() + static_cast<int>(end_number)};
}

auto Chord::copy_notes_to_json(size_t first_note_number,
                               size_t number_of_notes) const -> nlohmann::json {
  check_note_number(first_note_number);

  auto end_number = first_note_number + number_of_notes;
  check_new_note_number(end_number);

  nlohmann::json json_notes;
  std::transform(notes.cbegin() + static_cast<int>(first_note_number),
                 notes.cbegin() + static_cast<int>(end_number),
                 std::back_inserter(json_notes),
                 [](const Note &note) { return note.json(); });
  return json_notes;
}

void Chord::insert_notes(size_t first_note_number,
                         const std::vector<Note> &new_notes) {
  check_new_note_number(first_note_number);
  notes.insert(notes.begin() + static_cast<int>(first_note_number),
               new_notes.begin(), new_notes.end());
};

void Chord::insert_json_notes(size_t first_note_number,
                              const nlohmann::json &json_notes) {
  check_new_note_number(first_note_number);
  std::transform(
      json_notes.cbegin(), json_notes.cend(),
      std::inserter(notes, notes.begin() + static_cast<int>(first_note_number)),
      [](const nlohmann::json &json_note) { return Note(json_note); });
}

void Chord::remove_notes(size_t first_note_number, size_t number_of_notes) {
  check_note_number(first_note_number);

  auto end_number = first_note_number + number_of_notes;
  check_new_note_number(end_number);

  notes.erase(notes.begin() + static_cast<int>(first_note_number),
              notes.begin() + static_cast<int>(end_number));
}

auto get_chord_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema = []() {
    auto chord_properties = get_note_chord_fields_schema();
    chord_properties["notes"] = get_notes_schema();
    return nlohmann::json({{"type", "object"},
                           {"description", "a chord"},
                           {"properties", chord_properties}});
  }();
  return chord_schema;
}
