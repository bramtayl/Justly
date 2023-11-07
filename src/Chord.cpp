#include "justly/Chord.h"

#include <algorithm>
#include <map>                           // for operator!=, operator==
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json

#include "justly/Interval.h"   // for Interval
#include "justly/Note.h"       // for Note
#include "justly/NoteChord.h"  // for NoteChord, TreeLevel, chord_level

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    insert_json_chilren(0, json_chord["notes"]);
  }
}

auto Chord::symbol_for() const -> std::string { return "♫"; }

auto Chord::get_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema(
      {{"type", "object"},
       {"description", "a chord"},
       {"properties",
        {{"interval", Interval::get_schema()},
         {"tempo_percent", NoteChord::get_tempo_percent_schema()},
         {"volume_percent", NoteChord::get_volume_percent_schema()},
         {"beats", NoteChord::get_beats_schema()},
         {"words", NoteChord::get_words_schema()},
         {"instrument", NoteChord::get_instrument_schema()},
         {"notes",
          {{"type", "array"},
           {"description", "the notes"},
           {"items", Note::get_schema()}}}}}});
  return chord_schema;
}

auto Chord::to_json() const -> nlohmann::json {
  auto json_chord = NoteChord::to_json();
  if (!note_pointers.empty()) {
    json_chord["notes"] =
        children_to_json(0, static_cast<int>(note_pointers.size()));
  }
  return json_chord;
}

auto Chord::children_to_json(int first_note_number, int number_of_notes) const
    -> nlohmann::json {
  nlohmann::json json_notes;
  for (int note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    json_notes.push_back(note_pointers[note_number]->to_json());
  }
  return json_notes;
}

void Chord::insert_empty_chilren(int first_note_number, int number_of_notes) {
  for (int note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    // will error if childless
    note_pointers.insert(note_pointers.begin() + note_number,
                         std::make_unique<Note>());
  }
}

void Chord::remove_children(int first_note_number, int number_of_notes) {
  note_pointers.erase(
      note_pointers.begin() + first_note_number,
      note_pointers.begin() + first_note_number + number_of_notes);
}

void Chord::insert_json_chilren(int first_note_number,
                              const nlohmann::json &json_children) {
  for (int insertion_number = 0;
       insertion_number < static_cast<int>(json_children.size());
       insertion_number = insertion_number + 1) {
    note_pointers.insert(
        note_pointers.begin() + first_note_number + insertion_number,
        std::make_unique<Note>(json_children[insertion_number]));
  }
}
