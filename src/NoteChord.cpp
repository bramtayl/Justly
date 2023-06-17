#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for QFlags
#include <qcolor.h>          // for QColor
#include <qjsonvalue.h>      // for QJsonValueRef

NoteChord::NoteChord(const QString &default_instrument)
    : default_instrument(default_instrument){

      };

auto NoteChord::save(QJsonObject &json_map) const -> void {
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
};

void NoteChord::load(const QJsonObject &json_note_chord) {
  numerator = get_json_int(json_note_chord, "numerator", DEFAULT_NUMERATOR);
  denominator =
      get_json_int(json_note_chord, "denominator", DEFAULT_DENOMINATOR);
  octave = get_json_int(json_note_chord, "octave", DEFAULT_OCTAVE);
  beats = get_json_int(json_note_chord, "beats", DEFAULT_BEATS);
  volume_percent = get_json_double(json_note_chord, "volume_percent",
                                   DEFAULT_VOLUME_PERCENT);
  tempo_percent =
      get_json_double(json_note_chord, "tempo_percent", DEFAULT_TEMPO_PERCENT);
  words = get_json_string(json_note_chord, "words", "");
}

auto NoteChord::setData(int column, const QVariant &new_value) -> bool {
  if (column == numerator_column) {
    numerator = new_value.toInt();
    return true;
  };
  if (column == denominator_column) {
    denominator = new_value.toInt();
    return true;
  };
  if (column == octave_column) {
    octave = new_value.toInt();
    return true;
  };
  if (column == beats_column) {
    beats = new_value.toInt();
    return true;
  };
  if (column == volume_percent_column) {
    volume_percent = new_value.toDouble();
    return true;
  };
  if (column == tempo_percent_column) {
    tempo_percent = new_value.toDouble();
    return true;
  };
  if (column == words_column) {
    words = new_value.toString();
    return true;
  };
  return false;
}

auto NoteChord::get_value(int column) const -> QVariant {
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
    return QString("%1\%").arg(volume_percent);
  };
  if (column == tempo_percent_column) {
    return QString("%1\%").arg(tempo_percent);
  };
  if (column == words_column) {
    return words;
  };
  return {};
}

auto NoteChord::get_color(int column) const -> QVariant {
  if (column == symbol_column) {
    return NON_DEFAULT_COLOR;
  }
  if (column == numerator_column) {
    if (numerator == DEFAULT_NUMERATOR) {
      return DEFAULT_COLOR;
    }
    return NON_DEFAULT_COLOR;
  };
  if (column == denominator_column) {
    if (denominator == DEFAULT_DENOMINATOR) {
      return DEFAULT_COLOR;
    }
    return NON_DEFAULT_COLOR;
  };
  if (column == octave_column) {
    if (octave == DEFAULT_OCTAVE) {
      return DEFAULT_COLOR;
    }
    return NON_DEFAULT_COLOR;
  };
  if (column == beats_column) {
    if (beats == DEFAULT_BEATS) {
      return DEFAULT_COLOR;
    }
    return NON_DEFAULT_COLOR;
  };
  if (column == volume_percent_column) {
    if (volume_percent == DEFAULT_VOLUME_PERCENT) {
      return DEFAULT_COLOR;
    }
    return NON_DEFAULT_COLOR;
  };
  if (column == tempo_percent_column) {
    if (tempo_percent == DEFAULT_TEMPO_PERCENT) {
      return DEFAULT_COLOR;
    }
    return NON_DEFAULT_COLOR;
  };
  if (column == words_column) {
    if (words == "") {
      return DEFAULT_COLOR;
    };
    return NON_DEFAULT_COLOR;
  }
  return {};
}

auto NoteChord::flags(int column) const -> Qt::ItemFlags {
  if (column == symbol_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
  if (column == numerator_column || column == denominator_column ||
      column == octave_column || column == beats_column ||
      column == volume_percent_column || column == tempo_percent_column ||
      column == words_column) {
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  }
  return Qt::NoItemFlags;
}