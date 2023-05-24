#include "Note.h"

#include <QtCore/qglobal.h>  // for QFlags, qCritical
#include <qcolor.h>          // for QColor
#include <qjsonvalue.h>      // for QJsonValue, QJsonValueRef
#include <qstring.h>         // for QString

#include "JsonHelpers.h"

Note::Note() : NoteChord() {}

auto Note::get_level() const -> int { return NOTE_LEVEL; };

auto Note::flags(int column) const -> Qt::ItemFlags {
  if (column == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  if (column == numerator_column || column == denominator_column ||
      column == octave_column || column == beats_column ||
      column == volume_ratio_column || column == tempo_ratio_column ||
      column == words_column || column == instrument_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  NoteChord::error_column(column);
  return Qt::NoItemFlags;
}

void Note::load(const QJsonObject &json_note_chord) {
  numerator = get_positive_int(json_note_chord, "numerator", DEFAULT_NUMERATOR);
  denominator =
      get_positive_int(json_note_chord, "denominator", DEFAULT_DENOMINATOR);
  octave = get_int(json_note_chord, "octave", DEFAULT_OCTAVE);
  beats = get_positive_int(json_note_chord, "beats", DEFAULT_BEATS);
  volume_ratio = get_positive_double(json_note_chord, "volume_ratio",
                                     DEFAULT_VOLUME_RATIO);
  tempo_ratio =
      get_positive_double(json_note_chord, "tempo_ratio", DEFAULT_TEMPO_RATIO);
  words = get_string(json_note_chord, "words", "");
  instrument = get_string(json_note_chord, "instrument", DEFAULT_INSTRUMENT);
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
  if (volume_ratio != DEFAULT_VOLUME_RATIO) {
    json_map["volume_ratio"] = volume_ratio;
  }
  if (tempo_ratio != DEFAULT_TEMPO_RATIO) {
    json_map["tempo_ratio"] = tempo_ratio;
  }
  if (words != "") {
    json_map["words"] = words;
  }
  if (instrument != DEFAULT_INSTRUMENT) {
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
    if (column == volume_ratio_column) {
      return volume_ratio;
    };
    if (column == tempo_ratio_column) {
      return tempo_ratio;
    };
    if (column == words_column) {
      return words;
    };
    if (column == instrument_column) {
      return instrument;
    }
    NoteChord::error_column(column);
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
    if (column == volume_ratio_column) {
      if (volume_ratio == DEFAULT_VOLUME_RATIO) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == tempo_ratio_column) {
      if (tempo_ratio == DEFAULT_TEMPO_RATIO) {
        return QColor(Qt::lightGray);
      }
      return {};
    };
    if (column == words_column) {
      return words;
    };
    if (column == instrument_column) {
      if (instrument == DEFAULT_INSTRUMENT) {
        return QColor(Qt::lightGray);
      }
      return {};
    }
    NoteChord::error_column(column);
  }
  // no data for other roles
  return {};
}

auto Note::setData(int column, const QVariant &new_value, int role) -> bool {
  if (role == Qt::EditRole) {
    if (column == numerator_column) {
      auto new_numerator = new_value.toInt();
      if (new_numerator > 0) {
        numerator = new_numerator;
        return true;
      }
      return false;
    };
    if (column == denominator_column) {
      auto new_denominator = new_value.toInt();
      if (new_denominator > 0) {
        denominator = new_denominator;
        return true;
      }
      return false;
    };
    if (column == octave_column) {
      octave = new_value.toInt();
      return true;
    };
    if (column == beats_column) {
      auto parsed = new_value.toInt();
      // beats cant be negative
      if (parsed >= 0) {
        beats = parsed;
        return true;
      }
      return false;
    };
    if (column == volume_ratio_column) {
      auto new_volume_ratio = new_value.toFloat();
      if (new_volume_ratio > 0) {
        volume_ratio = new_volume_ratio;
        return true;
      }
      return false;
    };
    if (column == tempo_ratio_column) {
      auto new_tempo_ratio = new_value.toFloat();
      if (new_tempo_ratio > 0) {
        tempo_ratio = new_tempo_ratio;
        return true;
      }
      return false;
    };
    if (column == words_column) {
      words = new_value.toString();
      return true;
    };
    if (column == instrument_column) {
      instrument = new_value.toString();
      return true;
    };
    NoteChord::error_column(column);
  };
  // dont set any other role
  return false;
}

auto Note::copy_pointer() -> std::unique_ptr<NoteChord> {
  return std::make_unique<Note>(*this);
}
