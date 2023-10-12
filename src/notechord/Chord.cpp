#include "notechord/Chord.h"

#include <qstring.h>  // for QString

#include <map>                           // for operator!=, operator==
#include <nlohmann/detail/json_ref.hpp>  // for json_ref
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json

#include "metatypes/Interval.h"   // for Interval
#include "notechord/Note.h"       // for Note
#include "notechord/NoteChord.h"  // for NoteChord, TreeLevel, chord_level

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::get_schema() -> const nlohmann::json& {
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
