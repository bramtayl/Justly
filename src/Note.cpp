#include "Note.h"

#include <qcontainerfwd.h>  // for QStringList
#include <qjsonobject.h>
#include <qlist.h>          // for QList, QList<>::iterator
#include <qstring.h>        // for QString

#include "NoteChord.h"  // for error_level, note_level, TreeLevel
#include "utilities.h"

class Song;

Note::Note() : NoteChord() {}

auto Note::symbol_for() const -> QString { return "♪"; }

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::new_child_pointer() -> std::unique_ptr<NoteChord> {
  error_level(note_level);
  return nullptr;
}

auto Note::verify_json(const Song& song, const QJsonValue &note_value) -> bool {
  if (!(verify_json_object(note_value, "note"))) {
    return false;
  }
  auto json_note = note_value.toObject();
  for (const auto &field_name : json_note.keys()) {
    if (!(NoteChord::verify_json_field(song, json_note, field_name))) {
      return false;
    }
  };
  return true;
}
