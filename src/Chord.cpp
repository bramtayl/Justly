#include "Chord.h"

#include <qcontainerfwd.h>  // for QStringList
#include <qjsonarray.h>     // for QJsonArray, QJsonArray::const_iterator
#include <qjsonvalue.h>     // for QJsonValueConstRef, QJsonValue
#include <qlist.h>          // for QList, QList<>::iterator
#include <qstring.h>        // for QString

#include "Note.h"       // for Note
#include "Utilities.h"  // for verify_json_array, verify_json_object

class Instrument;

Chord::Chord() : NoteChord() {}

auto Chord::symbol_for() const -> QString { return "♫"; }

auto Chord::get_level() const -> TreeLevel { return chord_level; }

auto Chord::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Chord>(*this);
}

auto Chord::new_child_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>();
}

auto Chord::verify_json(const QJsonObject &json_chord,
                        const std::vector<Instrument> &instruments) -> bool {
  for (const auto &field_name : json_chord.keys()) {
    if (field_name == "notes") {
      const auto notes_object = json_chord[field_name];
      if (!verify_json_array(notes_object, "notes")) {
        return false;
      }
      const auto json_notes = notes_object.toArray();
      for (const auto &note_value : json_notes) {
        if (!verify_json_object(note_value, "note")) {
          return false;
        }
        if (!(Note::verify_json(note_value.toObject(), instruments))) {
          return false;
        }
      }
    } else if (!(NoteChord::verify_json_note_chord_field(json_chord, field_name,
                                                         instruments))) {
      return false;
    }
  }
  return true;
}
