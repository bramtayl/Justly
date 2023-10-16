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

#include "metatypes/Instrument.h"  // for Instrument
#include "metatypes/Interval.h"    // for Interval

NoteChord::NoteChord(const nlohmann::json& json_object)
    : interval(json_object.contains("interval")
                   ? Interval(json_object["interval"])
                   : Interval()),
      beats(json_object.value("beats", DEFAULT_BEATS)),
      volume_percent(
          json_object.value("volume_percent", DEFAULT_VOLUME_PERCENT)),
      tempo_percent(json_object.value("tempo_percent", DEFAULT_TEMPO_PERCENT)),
      words(QString::fromStdString(json_object.value("words", DEFAULT_WORDS))),
      instrument_pointer(&Instrument::get_instrument_by_name(
          json_object.value("instrument", ""))) {}

auto NoteChord::to_json() const -> nlohmann::json {
  auto json_map = nlohmann::json::object();
  if (!(interval.is_default())) {
    json_map["interval"] = interval.to_json();
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
  return json_map;
}

void NoteChord::setData(NoteChordField column, const QVariant& new_value) {
  switch (column) {
    case interval_column:
      interval = new_value.value<Interval>();
      return;
    case beats_column:
      beats = new_value.toInt();
      return;
    case volume_percent_column:
      volume_percent = new_value.toDouble();
      return;
    case tempo_percent_column:
      tempo_percent = new_value.toDouble();
      return;
    case words_column:
      words = new_value.toString();
      return;
    case instrument_column:
      instrument_pointer = new_value.value<const Instrument*>();
      return;
    default:  // symbol_column
      return;
  }
}

auto NoteChord::data(NoteChordField column, Qt::ItemDataRole role) const
    -> QVariant {
  switch (column) {
    case symbol_column:
      switch (role) {
        case Qt::DisplayRole:
          return symbol_for();
        case Qt::ForegroundRole:
          return NON_DEFAULT_COLOR;
        default:
          break;
      }
      break;
    case interval_column:
      switch (role) {
        case Qt::DisplayRole:
          return interval.get_text();
        case Qt::EditRole:
          return QVariant::fromValue(interval);
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(interval.is_default());
        default:
          break;
      }
      break;
    case (beats_column):
      switch (role) {
        case Qt::DisplayRole:
          return beats;
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(beats == DEFAULT_BEATS);
        case Qt::EditRole:
          return beats;
        default:
          break;
      }
      break;
    case volume_percent_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1%").arg(volume_percent);
        case Qt::EditRole:
          return volume_percent;
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(volume_percent ==
                                           DEFAULT_VOLUME_PERCENT);
        default:
          break;
      }
      break;
    case tempo_percent_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString("%1%").arg(tempo_percent);
        case Qt::EditRole:
          return tempo_percent;
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(tempo_percent ==
                                           DEFAULT_TEMPO_PERCENT);
        default:
          break;
      }
      break;
    case words_column:
      switch (role) {
        case Qt::DisplayRole:
          return words;
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(words == DEFAULT_WORDS);
        case Qt::EditRole:
          return words;
        default:
          break;
      }
      break;
    default:  // instrument_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(instrument_pointer->instrument_name);
        case Qt::EditRole:
          return QVariant::fromValue(instrument_pointer.get());
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(
              instrument_pointer->instrument_name.empty());
        default:
          break;
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

auto NoteChord::get_text_color(bool is_default) -> QColor {
  return is_default ? DEFAULT_COLOR : NON_DEFAULT_COLOR;
}
