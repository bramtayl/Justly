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
    from_json(&note_pointers, 0, json_chord["notes"]);
  }
}

auto Chord::symbol() const -> std::string { return "♫"; }

auto Chord::json_schema() -> const nlohmann::json & {
  static const nlohmann::json chord_schema(
      {{"type", "object"},
       {"description", "a chord"},
       {"properties",
        {{"interval", Interval::json_schema()},
         {"tempo_percent", NoteChord::tempo_percent_schema()},
         {"volume_percent", NoteChord::volume_percent_schema()},
         {"beats", NoteChord::beats_schema()},
         {"words", NoteChord::words_schema()},
         {"instrument", NoteChord::instrument_schema()},
         {"notes",
          {{"type", "array"},
           {"description", "the notes"},
           {"items", Note::json_schema()}}}}}});
  return chord_schema;
}

auto Chord::json() const -> nlohmann::json {
  auto json_chord = NoteChord::json();
  if (!note_pointers.empty()) {
    json_chord["notes"] = to_json(
        note_pointers, 0, static_cast<int>(note_pointers.size()));
  }
  return json_chord;
}

