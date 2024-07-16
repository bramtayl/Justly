#include "justly/Chord.hpp"

#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <QtGlobal>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    notes_from_json(0, json_chord["notes"]);
  }
}

auto Chord::symbol() const -> std::string { return "♫"; }

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!notes.empty()) {
    json_chord["notes"] = notes_to_json(0, notes.size());
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

auto Chord::notes_to_json(size_t first_child_number,
                          size_t number_of_children) const -> nlohmann::json {
  auto notes_size = notes.size();
  Q_ASSERT(first_child_number < notes_size);
  auto end_number = first_child_number + number_of_children;
  Q_ASSERT(end_number <= notes_size);
  nlohmann::json json_notes;
  std::transform(notes.cbegin() + static_cast<int>(first_child_number),
                 notes.cbegin() + static_cast<int>(end_number),
                 std::back_inserter(json_notes),
                 [](const Note &note) { return note.json(); });
  return json_notes;
}

void Chord::notes_from_json(size_t first_child_number,
                            const nlohmann::json &json_notes) {
  Q_ASSERT(first_child_number <= notes.size());
  std::transform(
      json_notes.cbegin(), json_notes.cend(),
      std::inserter(notes,
                    notes.begin() + static_cast<int>(first_child_number)),
      [](const nlohmann::json &json_note) { return Note(json_note); });
}
