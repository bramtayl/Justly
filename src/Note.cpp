#include "justly/Note.h"

#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, basi...
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>

#include "justly/Interval.h"   // for Interval
#include "justly/NoteChord.h"  // for NoteChord, TreeLevel, note_l...

Note::Note(const nlohmann::json& json_note) : NoteChord(json_note) {}

auto Note::symbol() const -> std::string { return "♪"; }

auto Note::json_schema() -> const nlohmann::json& {
  static const nlohmann::json note_schema(
      {{"type", "object"},
       {"description", "a note"},
       {"properties",
        {{"interval", Interval::json_schema()},
         {"tempo_percent", NoteChord::tempo_percent_schema()},
         {"volume_percent", NoteChord::volume_percent_schema()},
         {"beats", NoteChord::beats_schema()},
         {"words", NoteChord::words_schema()},
         {"instrument", NoteChord::instrument_schema()}}}});
  return note_schema;
}
