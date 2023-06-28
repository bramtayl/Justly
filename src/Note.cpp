#include "Note.h"

#include <qcontainerfwd.h>   // for QStringList
#include <qlist.h>           // for QList, QList<>::iterator
#include <qstring.h>         // for QString, operator!=, operator==

#include "Utilities.h"  // for error_column, verify_bounded_int, verify...

Note::Note() : NoteChord() {
  
}

auto Note::symbol_for() const -> QString {
  return "♪";
}

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>(*this);
}

auto Note::new_child_pointer() -> std::unique_ptr<NoteChord> {
  error_level(note_level);
  return nullptr;
}

auto Note::verify_json(
    const QJsonObject &json_note,
    const std::vector<Instrument> &instruments)
    -> bool {
  for (const auto &field_name : json_note.keys()) {
     if (!(NoteChord::verify_json_note_chord_field(json_note, field_name, instruments))) {
      return false;

    }
  };
  return true;
}
