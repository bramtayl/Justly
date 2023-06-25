#include "NoteChord.h"

#include <qcolor.h>     // for QColor
#include <qjsonvalue.h> // for QJsonValueRef
#include <qnamespace.h> // for DisplayRole, ForegroundRole
#include <utility>      // for move

auto NoteChord::save(QJsonObject &json_map) const -> void {
  if (!(interval.is_default())) {
    json_map["interval"] = interval.get_text();
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
  if (words != DEFAULT_WORDS) {
    json_map["words"] = words;
  }
  if (instrument != DEFAULT_WORDS) {
    json_map["instrument"] = instrument;
  }
};

void NoteChord::load(const QJsonObject &json_note_chord) {
  if (json_note_chord.contains("interval")) {
    interval.set_text(json_note_chord["interval"].toString());
  }
  beats = get_json_int(json_note_chord, "beats", DEFAULT_BEATS);
  volume_percent = get_json_double(json_note_chord, "volume_percent",
                                   DEFAULT_VOLUME_PERCENT);
  tempo_percent =
      get_json_double(json_note_chord, "tempo_percent", DEFAULT_TEMPO_PERCENT);
  words = get_json_string(json_note_chord, "words", DEFAULT_WORDS);
  instrument = get_json_string(json_note_chord, "instrument", "");
}

auto NoteChord::setData(int column, const QVariant &new_value) -> bool {
  if (column == interval_column) {
    interval.set_text(new_value.toString());
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
  if (column == instrument_column) {
    instrument = new_value.toString();
    return true;
  };
  error_column(column);
  return false;
}

auto NoteChord::data(int column, int role) const -> QVariant {
  if (role == Qt::DisplayRole) {
    if (column == symbol_column) {
      return symbol_for();
    }
    if (column == interval_column) {
      return interval.get_text();
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
    if (column == instrument_column) {
      return instrument;
    }
    error_column(column);
  };
  if (role == Qt::ForegroundRole) {
    if (column == symbol_column) {
      return NON_DEFAULT_COLOR;
    }
    if (column == interval_column) {
      if (interval.is_default()) {
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
      if (words == DEFAULT_WORDS) {
        return DEFAULT_COLOR;
      };
      return NON_DEFAULT_COLOR;
    }
    if (column == instrument_column) {
      if (instrument == "") {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    error_column(column);
  }
  // no data for other roles
  return {};
}

auto NoteChord::verify_json_note_chord_field(
    const QJsonObject &json_note_chord, const QString &field_name,
    const std::vector<std::unique_ptr<const QString>> &new_instrument_pointers)
    -> bool {
  if (field_name == "interval") {
    auto interval_value = json_note_chord["interval"];
    if (!(verify_json_string(interval_value, field_name))) {
      return false;
    }
    if (!(Interval::verify_json(interval_value.toString()))) {
      return false;
    }
  } else if (field_name == "beats") {
    if (!(verify_bounded_int(json_note_chord, field_name, MINIMUM_BEATS,
                             MAXIMUM_BEATS))) {
      qInfo("here");
      return false;
    }
  } else if (field_name == "volume_percent") {
    if (!(verify_bounded_double(json_note_chord, field_name,
                                MINIMUM_VOLUME_PERCENT,
                                MAXIMUM_VOLUME_PERCENT))) {
      return false;
    }
  } else if (field_name == "tempo_percent") {
    if (!(verify_bounded_double(json_note_chord, field_name,
                                MINIMUM_TEMPO_PERCENT,
                                MAXIMUM_TEMPO_PERCENT))) {
      return false;
    }
  } else if (field_name == "words") {
    if (!(verify_json_string(json_note_chord["words"], field_name))) {
      return false;
    }
  } else if (field_name == "instrument") {
    if (!verify_json_instrument(new_instrument_pointers, json_note_chord,
                                field_name, true)) {
      return false;
    }
  } else {
    warn_unrecognized_field("note", field_name);
    return false;
  }
  return true;
}