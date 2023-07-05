#include "Chord.h"

#include <qcontainerfwd.h>  // for QStringList
#include <qjsonarray.h>     // for QJsonArray, QJsonArray::const_iterator
#include <qjsonobject.h>
#include <qjsonvalue.h>     // for QJsonValueConstRef, QJsonValue
#include <qlist.h>          // for QList, QList<>::iterator
#include <qstring.h>        // for QString

#include "Note.h"           // for Note
#include "src/NoteChord.h"  // for NoteChord, TreeLevel, chord_level
#include "utilities.h"      // for verify_json_array, verify_json_object

class Instrument;

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::verify_json(const QJsonValue &chord_value,
                        const std::vector<Instrument> &instruments) -> bool {
  if (!(verify_json_object(chord_value, "chord"))) {
    return false;
  }
  auto chord_object = chord_value.toObject();
  for (const auto &field_name : chord_object.keys()) {
    if (field_name == "notes") {
      const auto notes_object = chord_object[field_name];
      if (!verify_json_array(notes_object, "notes")) {
        return false;
      }
      const auto notes_array = notes_object.toArray();
      for (const auto &note_value : notes_array) {
        if (!(Note::verify_json(note_value, instruments))) {
          return false;
        }
      }
    } else if (!(NoteChord::verify_json_field(chord_object, field_name,
                                                         instruments))) {
      return false;
    }
  }
  return true;
}
