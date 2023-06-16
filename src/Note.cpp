#include "Note.h"

#include <QtCore/qglobal.h>  // for QFlags
#include <qcolor.h>          // for QColor
#include <qjsonvalue.h>      // for QJsonValueRef
#include <qstring.h>         // for QString, operator!=, operator==

#include "Utilities.h"  // for get_json_int, error_column, get...

Note::Note(const QString &default_instrument)
    : NoteChord(default_instrument){

      };

auto Note::get_level() const -> TreeLevel { return note_level; };

auto Note::flags(int column) const -> Qt::ItemFlags {
  if (column == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  if (column == numerator_column || column == denominator_column ||
      column == octave_column || column == beats_column ||
      column == volume_percent_column || column == tempo_percent_column ||
      column == words_column || column == instrument_column) {
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
    auto generic_value = NoteChord::get_value(column);
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
    if (column == symbol_column) {
      return {};
    }
    if (column == numerator_column) {
      if (numerator == DEFAULT_NUMERATOR) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == denominator_column) {
      if (denominator == DEFAULT_DENOMINATOR) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == octave_column) {
      if (octave == DEFAULT_OCTAVE) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == beats_column) {
      if (beats == DEFAULT_BEATS) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == volume_percent_column) {
      if (volume_percent == DEFAULT_VOLUME_PERCENT) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == tempo_percent_column) {
      if (tempo_percent == DEFAULT_TEMPO_PERCENT) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == words_column) {
      if (words == "") {
        return QColor(Qt::lightGray);
      };
      return {};
    }
    if (column == instrument_column) {
      if (instrument == default_instrument) {
        return QColor(Qt::lightGray);
      }
      return {};
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
