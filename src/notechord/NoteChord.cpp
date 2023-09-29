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

#include "metatypes/Instrument.h"      // for Instrument
#include "metatypes/Interval.h"        // for Interval
#include "metatypes/SuffixedNumber.h"  // for SuffixedNumber

auto NoteChord::save_to(nlohmann::json& json_map) const -> void {
  if (!(interval.is_default())) {
    nlohmann::json interval_map;
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
    json_map["words"] = words.toStdString();
  }
  if (get_instrument().instrument_name != "") {
    json_map["instrument"] = get_instrument().instrument_name.toStdString();
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
  set_instrument(Instrument::get_instrument_by_name(
      QString::fromStdString(json_note_chord.value("instrument", ""))));
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
    volume_percent = new_value.value<SuffixedNumber>().number;
    return;
  }
  if (column == tempo_percent_column) {
    tempo_percent = new_value.value<SuffixedNumber>().number;
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
  if (role == Qt::DisplayRole) {
    if (column == symbol_column) {
      return symbol_for();
    }
    if (column == interval_column) {
      return QVariant::fromValue(interval);
    }
    if (column == beats_column) {
      return beats;
    }
    if (column == volume_percent_column) {
      return QVariant::fromValue(SuffixedNumber(volume_percent, "%"));
    }
    if (column == tempo_percent_column) {
      return QVariant::fromValue(SuffixedNumber(tempo_percent, "%"));
    }
    if (column == words_column) {
      return words;
    }
    if (column == instrument_column) {
      return QVariant::fromValue(&(get_instrument()));
    }
  }
  if (role == Qt::ForegroundRole) {
    if (column == symbol_column) {
      return NON_DEFAULT_COLOR;
    }
    if (column == interval_column) {
      if (interval.is_default()) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    if (column == beats_column) {
      if (beats == DEFAULT_BEATS) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    if (column == volume_percent_column) {
      if (volume_percent == DEFAULT_VOLUME_PERCENT) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    if (column == tempo_percent_column) {
      if (tempo_percent == DEFAULT_TEMPO_PERCENT) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    if (column == words_column) {
      if (words == DEFAULT_WORDS) {
        return DEFAULT_COLOR;
      }
      return NON_DEFAULT_COLOR;
    }
    if (column == instrument_column) {
      if (get_instrument().instrument_name == "") {
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

auto NoteChord::get_instrument() const -> const Instrument& {
  return *instrument_pointer;
}

void NoteChord::set_instrument(const Instrument& new_instrument) {
  instrument_pointer = &new_instrument;
};