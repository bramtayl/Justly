#include "notechord/Chord.h"

#include <qstring.h>  // for QString

#include <map>                           // for operator!=, operator==
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>

#include "metatypes/Interval.h"   // for Interval
#include "notechord/Note.h"       // for Note
#include "notechord/NoteChord.h"  // for NoteChord, TreeLevel, chord_level

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

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

auto Chord::save_to(nlohmann::json *json_object_pointer) const -> void {
  NoteChord::save_to(json_object_pointer);
  if (!note_pointers.empty()) {
    nlohmann::json json_children;
    for (const auto &note_pointer : note_pointers) {
      nlohmann::json json_child = nlohmann::json::object();
      note_pointer->save_to(&json_child);
      json_children.push_back(std::move(json_child));
      auto &json_object = *json_object_pointer;
      json_object["chords"] = json_children;
    }
  }
}

auto Chord::copy_json_notes(int first_note_number, int number_of_notes) const
    -> nlohmann::json {
  nlohmann::json json_children;
  for (int index = first_note_number;
       index < first_note_number + number_of_notes; index = index + 1) {
    nlohmann::json json_child = nlohmann::json::object();
    note_pointers[index]->save_to(&json_child);
    json_children.push_back(std::move(json_child));
  }
  return json_children;
}

void Chord::insert_empty_notes(int first_note_number, int number_of_notes) {
  for (int index = first_note_number;
       index < first_note_number + number_of_notes; index = index + 1) {
    // will error if childless
    note_pointers.insert(note_pointers.begin() + index,
                         std::make_unique<Note>());
  }
}

void Chord::remove_notes(int first_note_number, int number_of_notes) {
  note_pointers.erase(
      note_pointers.begin() + first_note_number,
      note_pointers.begin() + first_note_number + number_of_notes);
}

void Chord::insert_json_notes(int first_note_number,
                              const nlohmann::json &insertion) {
  for (int offset = 0; offset < static_cast<int>(insertion.size());
       offset = offset + 1) {
    const auto &note_object = insertion[offset];
    auto new_note_pointer = std::make_unique<Note>();
    new_note_pointer->load_from(note_object);
    note_pointers.insert(note_pointers.begin() + first_note_number + offset,
                         std::move(new_note_pointer));
  }
}
