#include "justly/Chord.hpp"

#include <algorithm>              // for transform
#include <map>                    // for operator==, operator!=
#include <nlohmann/json.hpp>      // for basic_json<>::object_t, basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "justly/Note.hpp"       // for Note
#include "justly/NoteChord.hpp"  // for NoteChord
#include "song/objects.hpp"

Chord::Chord(const nlohmann::json &json_chord) : NoteChord(json_chord) {
  if (json_chord.contains("notes")) {
    from_json(&note_pointers, 0, json_chord["notes"]);
  }
}

auto Chord::symbol() const -> std::string { return "♫"; }

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!note_pointers.empty()) {
    json_chord["notes"] = to_json(
        note_pointers, 0, note_pointers.size());
  }
  return json_chord;
}
