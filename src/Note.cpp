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
  numerator =
      get_json_int(json_note_chord, "numerator", DEFAULT_NUMERATOR);
  denominator = get_json_int(json_note_chord, "denominator",
                                      DEFAULT_DENOMINATOR);
  octave = get_json_int(json_note_chord, "octave", DEFAULT_OCTAVE);
  beats = get_json_int(json_note_chord, "beats", DEFAULT_BEATS);
  volume_percent = get_json_double(json_note_chord, "volume_percent",
                                         DEFAULT_VOLUME_PERCENT);
  tempo_percent = get_json_double(json_note_chord, "tempo_percent",
                                        DEFAULT_TEMPO_PERCENT);
  words = get_json_string(json_note_chord, "words", "");
  instrument =
      get_json_string(json_note_chord, "instrument", default_instrument);
}

auto Note::save(QJsonObject &json_map) const -> void {
  if (numerator != DEFAULT_NUMERATOR) {
    json_map["numerator"] = numerator;
  }
  if (denominator != DEFAULT_DENOMINATOR) {
    json_map["denominator"] = denominator;
  }
  if (octave != DEFAULT_OCTAVE) {
    json_map["octave"] = octave;
  }
  if (beats != DEFAULT_BEATS) {
    json_map["beats"] = beats;
  }
  if (volume_percent != DEFAULT_VOLUME_PERCENT) {
    json_map["volume_percent"] = volume_percent;
  }
  if (tempo_percent != DEFAULT_TEMPO_PERCENT) {
    json_map["tempo_percent"] = tempo_percent;
  }
  if (words != "") {
    json_map["words"] = words;
  }
  if (instrument != default_instrument) {
    json_map["instrument"] = instrument;
  }
};

auto Note::data(int column, int role) const -> QVariant {
  if (role == Qt::DisplayRole) {
    if (column == symbol_column) {
      return "♪";
    }
    if (column == numerator_column) {
      return numerator;
    };
    if (column == denominator_column) {
      return denominator;
    };
    if (column == octave_column) {
      return octave;
    };
    if (column == beats_column) {
      return beats;
    };
    if (column == volume_percent_column) {
      return volume_percent;
    };
    if (column == tempo_percent_column) {
      return tempo_percent;
    };
    if (column == words_column) {
      return words;
    };
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

void Note::setData(int column, const QVariant &new_value) {
  if (column == numerator_column) {
    numerator = new_value.toInt();
    return;
  };
  if (column == denominator_column) {
    denominator = new_value.toInt();
    return;
  };
  if (column == octave_column) {
    octave = new_value.toInt();
    return;
  };
  if (column == beats_column) {
    beats = new_value.toInt();
    return;
  };
  if (column == volume_percent_column) {
    volume_percent = new_value.toInt();
    return;
  };
  if (column == tempo_percent_column) {
    tempo_percent = new_value.toInt();
    return;
  };
  if (column == words_column) {
    words = new_value.toString();
    return;
  };
  if (column == instrument_column) {
    instrument = new_value.toString();
    return;
  };
  error_column(column);
}

auto Note::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>(*this);
}

auto Note::get_instrument() -> QString { return instrument; }

auto Note::new_child_pointer() -> std::unique_ptr<NoteChord> {
  error_level(note_level);
  return nullptr;
} 
