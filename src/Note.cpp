#include "Note.h"

#include <QtCore/qglobal.h>  // for QFlags
#include <qcolor.h>          // for QColor
#include <qcontainerfwd.h>   // for QStringList
#include <qjsonvalue.h>      // for QJsonValueRef
#include <qlist.h>           // for QList, QList<>::iterator
#include <qstring.h>         // for QString, operator!=, operator==

#include "Utilities.h"  // for get_json_int, error_column, get...

Note::Note(const QString &default_instrument) : NoteChord(default_instrument){};

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::flags(int column) const -> Qt::ItemFlags {
  auto generic_flags = NoteChord::flags(column);
  if (generic_flags != Qt::NoItemFlags) {
    return generic_flags;
  }
  if (column == instrument_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  error_column(column);
  return Qt::NoItemFlags;
}

void Note::load(const QJsonObject &json_note_chord) {
  NoteChord::load(json_note_chord);
  instrument =
      get_json_string(json_note_chord, "instrument", default_instrument);
}

auto Note::save(QJsonObject &json_map) const -> void {
  NoteChord::save(json_map);
  if (instrument != default_instrument) {
    json_map["instrument"] = instrument;
  }
};

auto Note::data(int column, int role) const -> QVariant {
  if (role == Qt::DisplayRole) {
    if (column == symbol_column) {
      return "♪";
    }
    auto generic_value = NoteChord::get_value(column);
    if (generic_value != QVariant()) {
      return generic_value;
    }
    if (column == instrument_column) {
      return instrument;
    }
    error_column(column);
  };
  if (role == Qt::ForegroundRole) {
    auto generic_color = NoteChord::get_color(column);
    if (generic_color != QVariant()) {
      return generic_color;
    }
    if (column == instrument_column) {
      if (instrument == default_instrument) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    error_column(column);
  }
  // no data for other roles
  return {};
}

auto Note::setData(int column, const QVariant &new_value) -> bool {
  if (NoteChord::setData(column, new_value)) {
    return true;
  }
  if (column == instrument_column) {
    instrument = new_value.toString();
    return true;
  };
  error_column(column);
  return false;
}

auto Note::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>(*this);
}

auto Note::get_instrument() -> QString { return instrument; }

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
                                  "instrument")) {
        return false;
      }
    } else {
      warn_unrecognized_field("note", field_name);
      return false;
    }
  }
  return true;
}
