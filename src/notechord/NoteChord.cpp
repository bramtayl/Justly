#include "notechord/NoteChord.h"

#include <qcolor.h>      // for QColor
#include <qnamespace.h>  // for DisplayRole, ForegroundRole
#include <qstring.h>     // for QString
#include <qvariant.h>    // for QVariant

#include <gsl/pointers>                      // for not_null
#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string
#include <type_traits>

#include "metatypes/Instrument.h"      // for Instrument
#include "metatypes/Interval.h"        // for Interval

auto NoteChord::save_to(nlohmann::json* json_map_pointer) const -> void {
  auto& json_map = *json_map_pointer;
  if (!(interval.is_default())) {
    nlohmann::json interval_map;
    interval.save_to(&interval_map);
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
    json_map["words"] = words.toStdString();
  }
  const auto& instrument_name = instrument_pointer->instrument_name;
  if (!instrument_name.empty()) {
    json_map["instrument"] = instrument_name;
  }
}

void NoteChord::load_from(const nlohmann::json& json_note_chord) {
  if (json_note_chord.contains("interval")) {
    interval.load_from(json_note_chord["interval"]);
  }
  beats = json_note_chord.value("beats", DEFAULT_BEATS);
  volume_percent =
      json_note_chord.value("volume_percent", DEFAULT_VOLUME_PERCENT);
  tempo_percent = json_note_chord.value("tempo_percent", DEFAULT_TEMPO_PERCENT);
  words = QString::fromStdString(json_note_chord.value("words", DEFAULT_WORDS));
  instrument_pointer = &Instrument::get_instrument_by_name(
      json_note_chord.value("instrument", ""));
}

void NoteChord::setData(int column, const QVariant& new_value) {
  if (column == interval_column) {
    interval = new_value.value<Interval>();
    return;
  }
  if (column == beats_column) {
    beats = new_value.toInt();
    return;
  }
  if (column == volume_percent_column) {
    volume_percent = new_value.toDouble();
    return;
  }
  if (column == tempo_percent_column) {
    tempo_percent = new_value.toDouble();
    return;
  }
  if (column == words_column) {
    words = new_value.toString();
    return;
  }
  if (column == instrument_column) {
    instrument_pointer = new_value.value<const Instrument*>();
    return;
  }
}

auto NoteChord::data(int column, int role) const -> QVariant {
  if (column == symbol_column) {
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return symbol_for();
    }
    if (role == Qt::ForegroundRole) {
      return NON_DEFAULT_COLOR;
    }
  }
  if (column == interval_column) {
    if (role == Qt::DisplayRole) {
      return interval.get_text();
    }
    if (role == Qt::EditRole) {
      return QVariant::fromValue(interval);
    }
    if (role == Qt::ForegroundRole) {
      if (interval.is_default()) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
  }
  if (column == beats_column) {
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return beats;
    }
    if (role == Qt::ForegroundRole) {
      if (beats == DEFAULT_BEATS) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
  }
  if (column == volume_percent_column) {
    if (role == Qt::DisplayRole) {
      return QString("%1\%").arg(volume_percent);
    }
    if (role == Qt::EditRole) {
      return volume_percent;
    }
    if (role == Qt::ForegroundRole) {
      if (volume_percent == DEFAULT_VOLUME_PERCENT) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
  }
  if (column == tempo_percent_column) {
    if (role == Qt::DisplayRole) {
      return QString("%1\%").arg(tempo_percent);
    }
    if (role == Qt::EditRole) {
      return tempo_percent;
    }
    if (role == Qt::ForegroundRole) {
      if (tempo_percent == DEFAULT_TEMPO_PERCENT) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
  }
  if (column == words_column) {
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return words;
    }
    if (role == Qt::ForegroundRole) {
      if (words == DEFAULT_WORDS) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
  }
  if (column == instrument_column) {
    if (role == Qt::DisplayRole) {
      return QString::fromStdString(instrument_pointer->instrument_name);
    }
    if (role == Qt::EditRole) {
      return QVariant::fromValue(instrument_pointer.get());
    }
    if (role == Qt::ForegroundRole) {
      if (instrument_pointer->instrument_name.empty()) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
  }
  // no data for other roles
  return {};
}

auto NoteChord::get_instrument_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema(
      {{"type", "string"},
       {"description", "the instrument"},
       {"enum", Instrument::get_all_instrument_names()}});
  return instrument_schema;
}

auto NoteChord::get_beats_schema() -> nlohmann::json& {
  static nlohmann::json instrument_schema(
      {{"type", "integer"},
       {"description", "the number of beats"},
       {"minimum", MINIMUM_BEATS},
       {"maximum", MAXIMUM_BEATS}});
  return instrument_schema;
}

auto NoteChord::get_words_schema() -> nlohmann::json& {
  static nlohmann::json words_schema(
      {{"type", "string"}, {"description", "the words"}});
  return words_schema;
}

auto NoteChord::get_volume_percent_schema() -> nlohmann::json& {
  static nlohmann::json volume_percent_schema(
      {{"type", "number"},
       {"description", "the volume percent"},
       {"minimum", MINIMUM_VOLUME_PERCENT},
       {"maximum", MAXIMUM_VOLUME_PERCENT}});
  return volume_percent_schema;
}

auto NoteChord::get_tempo_percent_schema() -> nlohmann::json& {
  static nlohmann::json tempo_percent_schema(
      {{"type", "number"},
       {"description", "the tempo percent"},
       {"minimum", MINIMUM_TEMPO_PERCENT},
       {"maximum", MAXIMUM_TEMPO_PERCENT}});
  return tempo_percent_schema;
}

