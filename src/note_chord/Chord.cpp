#include "justly/Chord.hpp"

#include <QtGlobal>
#include <algorithm>
#include <iterator>
// IWYU pragma: no_include <map>
#include <nlohmann/json.hpp>
#include <utility>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"

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
    json_chord["notes"] = copy_notes_to_json(0, get_number_of_notes());
  }
  return json_chord;
}

auto Chord::get_number_of_notes() const -> size_t { return notes.size(); }

void Chord::check_note_number(size_t note_number) const {
  Q_ASSERT(note_number < get_number_of_notes());
}

void Chord::check_note_number_end(size_t note_number) const {
  Q_ASSERT(note_number <= get_number_of_notes());
}

void Chord::check_note_range(size_t first_note_number,
                             size_t number_of_notes) const {
  check_note_number(first_note_number);
  check_note_number_end(first_note_number + number_of_notes);
}

auto Chord::get_const_note(size_t note_number) const -> const Note & {
  check_note_number(note_number);
  return notes[note_number];
}

auto Chord::get_note(size_t note_number) -> Note & {
  check_note_number(note_number);
  return notes[note_number];
}

void Chord::set_note_data(size_t note_number, NoteChordColumn note_chord_column,
                          const QVariant &new_value) {
  get_note(note_number).setData(note_chord_column, new_value);
};

void Chord::copy_notes_to_notechords(size_t first_note_number, size_t number_of_notes,
                          std::vector<NoteChord> *note_chords_pointer) const {
  check_note_range(first_note_number, number_of_notes);
  Q_ASSERT(note_chords_pointer != nullptr);
  note_chords_pointer->insert(
      note_chords_pointer->end(),
      notes.cbegin() + static_cast<int>(first_note_number),
      notes.cbegin() + static_cast<int>(first_note_number + number_of_notes));
}

auto Chord::copy_notes(size_t first_note_number,
                       size_t number_of_notes) const -> std::vector<Note> {
  check_note_range(first_note_number, number_of_notes);
  return {notes.cbegin() + static_cast<int>(first_note_number),
          notes.cbegin() +
              static_cast<int>(first_note_number + number_of_notes)};
}

auto Chord::copy_notes_to_json(size_t first_note_number,
                               size_t number_of_notes) const -> nlohmann::json {
  check_note_range(first_note_number, number_of_notes);
  nlohmann::json json_notes;
  std::transform(notes.cbegin() + static_cast<int>(first_note_number),
                 notes.cbegin() +
                     static_cast<int>(first_note_number + number_of_notes),
                 std::back_inserter(json_notes),
                 [](const Note &note) { return note.to_json(); });
  return json_notes;
}

void Chord::insert_notes(size_t first_note_number,
                         const std::vector<Note> &new_notes) {
  check_note_number_end(first_note_number);
  notes.insert(notes.begin() + static_cast<int>(first_note_number),
               new_notes.begin(), new_notes.end());
};

void Chord::remove_notes(size_t first_note_number, size_t number_of_notes) {
  check_note_range(first_note_number, number_of_notes);
  notes.erase(notes.begin() + static_cast<int>(first_note_number),
              notes.begin() +
                  static_cast<int>(first_note_number + number_of_notes));
}

void Chord::replace_note_cells(size_t first_note_number,
                               size_t number_of_children,
                               NoteChordColumn left_field,
                               NoteChordColumn right_field,
                               const std::vector<NoteChord> &note_chords,
                               size_t first_note_chord_number) {
  for (size_t replace_number = 0; replace_number < number_of_children;
       replace_number = replace_number + 1) {
    auto note_chord_number = first_note_chord_number + replace_number;
    Q_ASSERT(note_chord_number < note_chords.size());
    get_note(first_note_number + replace_number)
        .replace_cells(left_field, right_field, note_chords[note_chord_number]);
  }
};

auto get_chords_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema = []() {
    auto chord_properties = get_note_chord_columns_schema();
    chord_properties["notes"] = get_notes_schema();
    return nlohmann::json({{"type", "array"},
                           {"description", "a list of chords"},
                           {"items",
                            {{"type", "object"},
                             {"description", "a chord"},
                             {"properties", std::move(chord_properties)}}}});
  }();
  return chord_schema;
}
