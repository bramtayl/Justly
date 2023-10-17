#include "notechord/NoteChord.h"

#include <qcolor.h>  // for QColor
#include <qfont.h>
#include <qnamespace.h>  // for DisplayRole, ForegroundRole
#include <qsize.h>
#include <qspinbox.h>
#include <qstring.h>   // for QString
#include <qvariant.h>  // for QVariant

#include <gsl/pointers>  // for not_null
#include <map>           // for operator!=, operator==
#include <memory>
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/detail/json_ref.hpp>      // for json_ref
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string
#include <type_traits>                       // for conditional_t

#include "editors/InstrumentEditor.h"
#include "editors/IntervalEditor.h"
#include "main/MyDelegate.h"
#include "metatypes/Instrument.h"  // for Instrument
#include "metatypes/Interval.h"    // for Interval

const auto WORDS_WIDTH = 200;
const auto SYMBOL_WIDTH = 20;

NoteChord::NoteChord(const nlohmann::json& json_note_chord)
    : interval(json_note_chord.contains("interval")
                   ? Interval(json_note_chord["interval"])
                   : Interval()),
      beats(json_note_chord.value("beats", DEFAULT_BEATS)),
      volume_percent(
          json_note_chord.value("volume_percent", DEFAULT_VOLUME_PERCENT)),
      tempo_percent(
          json_note_chord.value("tempo_percent", DEFAULT_TEMPO_PERCENT)),
      words(json_note_chord.value("words", DEFAULT_WORDS)),
      instrument_pointer(&Instrument::get_instrument_by_name(
          json_note_chord.value("instrument", ""))) {}

auto NoteChord::to_json() const -> nlohmann::json {
  auto json_note_chord = nlohmann::json::object();
  if (!(interval.is_default())) {
    json_note_chord["interval"] = interval.to_json();
  }
  if (beats != DEFAULT_BEATS) {
    json_note_chord["beats"] = beats;
  }
  if (volume_percent != DEFAULT_VOLUME_PERCENT) {
    json_note_chord["volume_percent"] = volume_percent;
  }
  if (tempo_percent != DEFAULT_TEMPO_PERCENT) {
    json_note_chord["tempo_percent"] = tempo_percent;
  }
  if (words != DEFAULT_WORDS) {
    json_note_chord["words"] = words;
  }
  const auto& instrument_name = instrument_pointer->instrument_name;
  if (!instrument_name.empty()) {
    json_note_chord["instrument"] = instrument_name;
  }
  return json_note_chord;
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
      words = new_value.toString().toStdString();
      return;
    case instrument_column:
      instrument_pointer = new_value.value<const Instrument*>();
      return;
    default:  // symbol_column
      return;
  }
}

auto NoteChord::get_cell_size(NoteChordField column) -> QSize {
  static auto beats_size = MyDelegate::create_beats_box(nullptr)->sizeHint();
  static auto interval_size =
      QSize(IntervalEditor().sizeHint().width(), beats_size.height());
  static auto volume_size = MyDelegate::create_volume_box(nullptr)->sizeHint();
  static auto tempo_size = MyDelegate::create_tempo_box(nullptr)->sizeHint();
  static auto instrument_size = InstrumentEditor().sizeHint();
  static auto words_size = QSize(WORDS_WIDTH, beats_size.height());
  static auto symbol_size = QSize(SYMBOL_WIDTH, beats_size.height());
  switch (column) {
    case beats_column:
      return beats_size;
    case interval_column:
      return interval_size;
    case volume_percent_column:
      return volume_size;
    case tempo_percent_column:
      return tempo_size;
    case instrument_column:
      return instrument_size;
    case words_column:
      return words_size;
    default:
      return symbol_size;
  }
}

auto NoteChord::data(NoteChordField column, Qt::ItemDataRole role) const
    -> QVariant {
  static auto larger_font = []() {
    QFont larger_font;
    larger_font.setPointSize(LARGE_FONT_SIZE);
    return larger_font;
  }();
  switch (column) {
    case symbol_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(symbol_for());
        case Qt::ForegroundRole:
          return NON_DEFAULT_COLOR;
        case Qt::SizeHintRole:
          return get_cell_size(column);
        case Qt::FontRole:
          return larger_font;
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
        case Qt::SizeHintRole:
          return get_cell_size(column);
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
        case Qt::SizeHintRole:
          return get_cell_size(column);
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
        case Qt::SizeHintRole:
          return get_cell_size(column);
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
        case Qt::SizeHintRole:
          return get_cell_size(column);
        default:
          break;
      }
      break;
    case words_column:
      switch (role) {
        case Qt::DisplayRole:
          return QString::fromStdString(words);
        case Qt::ForegroundRole:
          return NoteChord::get_text_color(words == DEFAULT_WORDS);
        case Qt::EditRole:
          return QString::fromStdString(words);
        case Qt::SizeHintRole:
          return get_cell_size(column);
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
        case Qt::SizeHintRole:
          return get_cell_size(column);
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
