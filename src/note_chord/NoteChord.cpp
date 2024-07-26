#include "justly/NoteChord.hpp"

#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <string>

#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/Rational.hpp"
#include "other/private.hpp"

auto get_rational_schema(const std::string &description) -> nlohmann::json {
  return nlohmann::json({{"type", "object"},
                         {"description", description},
                         {"properties",
                          {{"numerator",
                            {{"type", "integer"},
                             {"description", "the numerator"},
                             {"minimum", 1},
                             {"maximum", MAX_NUMERATOR}}},
                           {"denominator",
                            {{"type", "integer"},
                             {"description", "the denominator"},
                             {"minimum", 1},
                             {"maximum", MAX_DENOMINATOR}}}}}});
}

auto get_note_chord_fields_schema() -> const nlohmann::json & {
  static const nlohmann::json note_chord_fields_schema(
      {{"instrument", get_instrument_schema()},
       {"interval",
        {{"type", "object"},
         {"description", "an interval"},
         {"properties",
          {{"numerator",
            {{"type", "integer"},
             {"description", "the numerator"},
             {"minimum", 1},
             {"maximum", MAX_NUMERATOR}}},
           {"denominator",
            {{"type", "integer"},
             {"description", "the denominator"},
             {"minimum", 1},
             {"maximum", MAX_DENOMINATOR}}},
           {"octave",
            {{"type", "integer"},
             {"description", "the octave"},
             {"minimum", MIN_OCTAVE},
             {"maximum", MAX_OCTAVE}}}}}}},
       {"beats", get_rational_schema("the number of beats")},
       {"volume_percent", get_rational_schema("volume ratio")},
       {"tempo_percent", get_rational_schema("tempo ratio")},
       {"words", {{"type", "string"}, {"description", "the words"}}}});
  return note_chord_fields_schema;
}

NoteChord::NoteChord() : instrument_pointer(get_instrument_pointer("")) {}

NoteChord::NoteChord(const nlohmann::json &json_note_chord)
    : instrument_pointer(
          get_instrument_pointer(json_note_chord.value("instrument", ""))),
      interval(json_note_chord.contains("interval")
                   ? Interval(json_note_chord["interval"])
                   : Interval()),
      beats(json_note_chord.contains("beats")
                ? Rational(json_note_chord["beats"])
                : Rational()),
      volume_ratio(json_note_chord.contains("volume_ratio")
                       ? Rational(json_note_chord["volume_ratio"])
                       : Rational()),
      tempo_ratio(json_note_chord.contains("tempo_ratio")
                      ? Rational(json_note_chord["tempo_ratio"])
                      : Rational()),
      words(QString::fromStdString(json_note_chord.value("words", ""))) {}

auto NoteChord::symbol() const -> QString {
  Q_ASSERT(false);
  return "";
}

auto NoteChord::json() const -> nlohmann::json {
  auto json_note_chord = nlohmann::json::object();
  if (!(interval.is_default())) {
    json_note_chord["interval"] = interval.json();
  }
  if (!(beats.is_default())) {
    json_note_chord["beats"] = beats.json();
  }
  if (!(volume_ratio.is_default())) {
    json_note_chord["volume_ratio"] = volume_ratio.json();
  }
  if (!(tempo_ratio.is_default())) {
    json_note_chord["tempo_ratio"] = tempo_ratio.json();
  }
  if (!words.isEmpty()) {
    json_note_chord["words"] = words.toStdString().c_str();
  }
  Q_ASSERT(instrument_pointer != nullptr);
  if (!instrument_pointer->is_default()) {
    json_note_chord["instrument"] = instrument_pointer->instrument_name;
  }
  return json_note_chord;
}

auto NoteChord::data(NoteChordField note_chord_field,
                     int role) const -> QVariant {
  auto is_display = role == Qt::DisplayRole;
  auto is_display_or_edit = is_display || role == Qt::EditRole;
  switch (note_chord_field) {
  case type_column:
    if (is_display) {
      return symbol();
    }
    return {};
  case interval_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(interval);
    }
    return {};
  case (beats_column):
    if (is_display_or_edit) {
      return QVariant::fromValue(beats);
    }
    return {};
  case volume_ratio_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(volume_ratio);
    }
    return {};
  case tempo_ratio_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(tempo_ratio);
    }
    return {};
  case words_column:
    if (is_display_or_edit) {
      return words;
    }
    return {};
  case instrument_column:
    if (is_display_or_edit) {
      return QVariant::fromValue(instrument_pointer);
    }
    return {};
  default:
    Q_ASSERT(false);
    return {};
  }
};

void NoteChord::setData(NoteChordField note_chord_field,
                        const QVariant &new_value) {
  switch (note_chord_field) {
  case interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    interval = new_value.value<Interval>();
    break;
  case beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    beats = new_value.value<Rational>();
    break;
  case volume_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    volume_ratio = new_value.value<Rational>();
    break;
  case tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    tempo_ratio = new_value.value<Rational>();
    break;
  case words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    words = new_value.toString();
    break;
  case instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    instrument_pointer = new_value.value<const Instrument *>();
    break;
  default:
    Q_ASSERT(false);
  }
};

void NoteChord::replace_cells(NoteChordField left_field,
                              NoteChordField right_field,
                              const NoteChord &new_note_chord) {
  Q_ASSERT(right_field >= left_field);
  if (left_field <= instrument_column) {
    if (right_field < instrument_column) {
      return;
    }
    instrument_pointer = new_note_chord.instrument_pointer;
  }
  if (left_field <= interval_column) {
    if (right_field < interval_column) {
      return;
    }
    interval = new_note_chord.interval;
  }
  if (left_field <= beats_column) {
    if (right_field < beats_column) {
      return;
    }
    beats = new_note_chord.beats;
  }
  if (left_field <= volume_ratio_column) {
    if (right_field < volume_ratio_column) {
      return;
    }
    volume_ratio = new_note_chord.volume_ratio;
  }
  if (left_field <= tempo_ratio_column) {
    if (right_field < tempo_ratio_column) {
      return;
    }
    tempo_ratio = new_note_chord.tempo_ratio;
  }
  if (left_field <= words_column) {
    if (right_field < words_column) {
      return;
    }
    words = new_note_chord.words;
  }
}