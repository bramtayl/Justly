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
    const std::vector<std::unique_ptr<const QString>> &new_instrument_pointers)
    -> bool {
  for (const auto &field_name : json_note.keys()) {
    if (field_name == "numerator") {
      if (!(verify_bounded_int(json_note, field_name, MINIMUM_NUMERATOR,
                               MAXIMUM_NUMERATOR))) {
        return false;
      }
    } else if (field_name == "denominator") {
      if (!(verify_bounded_int(json_note, field_name, MINIMUM_DENOMINATOR,
                               MAXIMUM_DENOMINATOR))) {
        return false;
      }
    } else if (field_name == "octave") {
      if (!(verify_bounded_int(json_note, field_name, MINIMUM_OCTAVE,
                               MAXIMUM_OCTAVE))) {
        return false;
      }
    } else if (field_name == "beats") {
      if (!(verify_bounded_int(json_note, field_name, MINIMUM_BEATS,
                               MAXIMUM_BEATS))) {
        return false;
      }
    } else if (field_name == "volume_percent") {
      if (!(verify_bounded_double(json_note, field_name, MINIMUM_VOLUME_PERCENT,
                                  MAXIMUM_VOLUME_PERCENT))) {
        return false;
      }
    } else if (field_name == "tempo_percent") {
      if (!(verify_bounded_double(json_note, field_name, MINIMUM_TEMPO_PERCENT,
                                  MAXIMUM_TEMPO_PERCENT))) {
        return false;
      }
    } else if (field_name == "words") {
      if (!(verify_json_string(json_note["words"], field_name))) {
        return false;
      }
    } else if (field_name == "instrument") {
      if (!verify_json_instrument(new_instrument_pointers, json_note,
                                  "instrument", true)) {
        return false;
      }
    } else {
      warn_unrecognized_field("note", field_name);
      return false;
    }
  };
  return true;
}
