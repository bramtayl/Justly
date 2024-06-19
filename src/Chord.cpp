#include "justly/Chord.h"

#include <algorithm>              // for transform
#include <map>                    // for operator==, operator!=
#include <nlohmann/json.hpp>      // for basic_json<>::object_t, basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "justly/Note.h"       // for Note
#include "justly/NoteChord.h"  // for NoteChord
#include "src/json.h"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    objects_from_json(&note_pointers, 0, json_chord["notes"]);
  }
}

auto Chord::symbol() const -> std::string { return "♫"; }

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!note_pointers.empty()) {
    json_chord["notes"] = objects_to_json(
        note_pointers, 0, note_pointers.size());
  }
  return json_chord;
}
