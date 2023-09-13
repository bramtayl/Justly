#include "NoteChord.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <qcolor.h>          // for QColor
#include <qjsonvalue.h>      // for QJsonValueRef, QJsonValue
#include <qnamespace.h>      // for DisplayRole, ForegroundRole

#include "Instrument.h"
#include "Interval.h"        // for Interval
#include "SuffixedNumber.h"  // for SuffixedNumber
#include "utilities.h"       // for error_column, get_json_double, get_json_...

#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <nlohmann/detail/json_ref.hpp>  // for json_ref

auto NoteChord::save_to(QJsonObject &json_map) const -> void {
  if (!(interval.is_default())) {
    QJsonObject interval_map;
    interval.save_to(interval_map);
    json_map["interval"] = interval_map;
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

void NoteChord::load_from(const QJsonObject &json_note_chord) {
  if (json_note_chord.contains("interval")) {
    interval.load_from(json_note_chord["interval"].toObject());
  }
  beats = get_json_int(json_note_chord, "beats", DEFAULT_BEATS);
  volume_percent = get_json_double(json_note_chord, "volume_percent",
                                   DEFAULT_VOLUME_PERCENT);
  tempo_percent =
      get_json_double(json_note_chord, "tempo_percent", DEFAULT_TEMPO_PERCENT);
  words = get_json_string(json_note_chord, "words", DEFAULT_WORDS);
  instrument = get_json_string(json_note_chord, "instrument", "");
}

void NoteChord::setData(int column, const QVariant &new_value) {
  if (column == interval_column) {
    interval = qvariant_cast<Interval>(new_value);
    return;
  };
  if (column == beats_column) {
    beats = new_value.toInt();
    return;
  };
  if (column == volume_percent_column) {
    volume_percent = qvariant_cast<SuffixedNumber>(new_value).number;
    return;
  };
  if (column == tempo_percent_column) {
    tempo_percent = qvariant_cast<SuffixedNumber>(new_value).number;
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
}

auto NoteChord::data(int column, int role) const -> QVariant {
  if (role == Qt::DisplayRole) {
    if (column == symbol_column) {
      return symbol_for();
    }
    if (column == interval_column) {
      return QVariant::fromValue(interval);
    };
    if (column == beats_column) {
      return beats;
    };
    if (column == volume_percent_column) {
      return QVariant::fromValue(SuffixedNumber(volume_percent, "%"));
    };
    if (column == tempo_percent_column) {
      return QVariant::fromValue(SuffixedNumber(tempo_percent, "%"));
    };
    if (column == words_column) {
      return words;
    };
    if (column == instrument_column) {
      return instrument;
    }
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
  }
  // no data for other roles
  return {};
}

void error_level(TreeLevel level) { qCritical("Invalid level %d!", level); }

auto NoteChord::get_instrument_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema({
    {"type", "string"},
    {"description", "the instrument"},
    {"enum", Instrument::get_all_instrument_names()}
  });
  return instrument_schema;
}

auto NoteChord::get_beats_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema({
    {"type", "integer"},
    {"description", "the number of beats"},
    {"minimum", MINIMUM_BEATS},
    {"maximum", MAXIMUM_BEATS}
  });
  return instrument_schema;

}

auto NoteChord::get_words_schema() -> nlohmann::json& {
  static nlohmann::json words_schema({
    {"type", "string"},
    {"description", "the words"}
  });
  return words_schema;

}

auto NoteChord::get_volume_percent_schema() -> nlohmann::json& {
  static nlohmann::json volume_percent_schema({
    {"type", "number"},
    {"description", "the volume percent"},
    {"minimum", MINIMUM_VOLUME_PERCENT},
    {"maximum", MAXIMUM_VOLUME_PERCENT}
  });
  return volume_percent_schema;
}

auto NoteChord::get_tempo_percent_schema() -> nlohmann::json& {
  static nlohmann::json tempo_percent_schema({
    {"type", "number"},
    {"description", "the tempo percent"},
    {"minimum", MINIMUM_TEMPO_PERCENT},
    {"maximum", MAXIMUM_TEMPO_PERCENT}
  });
  return tempo_percent_schema;
}
