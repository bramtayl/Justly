#include "notechord/Note.h"

#include <map>                           // for operator!=
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>             // for basic_json<>::object_t, basi...
#include <nlohmann/json_fwd.hpp>         // for json
#include <string>

#include "metatypes/Interval.h"   // for Interval
#include "notechord/NoteChord.h"  // for NoteChord, TreeLevel, note_l...

Note::Note(const nlohmann::json& json_note)
    : NoteChord(json_note) {}

auto Note::symbol_for() const -> std::string { return "♪"; }

auto Note::get_schema() -> const nlohmann::json& {
  static const nlohmann::json note_schema(
      {{"type", "object"},
       {"description", "a note"},
       {"properties",
        {{"interval", Interval::get_schema()},
         {"tempo_percent", NoteChord::get_tempo_percent_schema()},
         {"volume_percent", NoteChord::get_volume_percent_schema()},
         {"beats", NoteChord::get_beats_schema()},
         {"words", NoteChord::get_words_schema()},
         {"instrument", NoteChord::get_instrument_schema()}}}});
  return note_schema;
}
