#include "Chord.h"

#include <qcontainerfwd.h>   // for QStringList
#include <qjsonarray.h>
#include <qjsonvalue.h>  // for QJsonValueConstRef, QJsonValue
#include <qlist.h>       // for QList, QList<>::iterator
#include <qstring.h>     // for QString

#include "Note.h"       // for Note
#include "Utilities.h"  // for error_column, TreeLevel, chord_level
#include "Interval.h"       // for Interval

Chord::Chord() : NoteChord() {
  
}

auto Chord::symbol_for() const -> QString {
  return "♫";
}

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Chord>(*this);
}

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::verify_json(
    const QJsonObject &json_chord,
    const std::vector<std::unique_ptr<const QString>> &new_instrument_pointers)
    -> bool {
  for (const auto &field_name : json_chord.keys()) {
    if (field_name == "interval") {
      if (!(verify_json_object(json_chord, "interval"))) {
        return false;
      }
      if (!(Interval::verify_json(json_chord["interval"].toObject()))) {
        return false;
      }
    } else if (field_name == "beats") {
      if (!(verify_bounded_int(json_chord, field_name, MINIMUM_BEATS,
                               MAXIMUM_BEATS))) {
        return false;
      }
    } else if (field_name == "volume_percent") {
      if (!(verify_bounded_double(json_chord, field_name,
                                  MINIMUM_VOLUME_PERCENT,
                                  MAXIMUM_VOLUME_PERCENT))) {
        return false;
      }
    } else if (field_name == "tempo_percent") {
      if (!(verify_bounded_double(json_chord, field_name, MINIMUM_TEMPO_PERCENT,
                                  MAXIMUM_TEMPO_PERCENT))) {
        return false;
      }
    } else if (field_name == "words") {
      if (!(verify_json_string(json_chord["words"], field_name))) {
        return false;
      }
    } else if (field_name == "instrument") {
      if (!verify_json_instrument(new_instrument_pointers, json_chord,
                                  "instrument", true)) {
        return false;
      }
    } else if (field_name == "notes") {
      const auto notes_object = json_chord[field_name];
      if (!verify_json_array(notes_object, "notes")) {
        return false;
      }
      const auto json_notes = notes_object.toArray();
      for (const auto &note_value : json_notes) {
        if (!verify_json_object(note_value, "note")) {
          return false;
        }
        if (!(Note::verify_json(note_value.toObject(), new_instrument_pointers))) {
          return false;
        }
      }
    } else {
      warn_unrecognized_field("chord", field_name);
      return false;
    }
  }
  return true;
}
