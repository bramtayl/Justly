#include "note_chord/NoteChord.hpp"

#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <string>

#include "cell_values/instruments.hpp"
#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "justly/Rational.hpp"

[[nodiscard]] auto
json_to_interval(const nlohmann::json &json_interval) -> Interval {
  return Interval(json_interval.value("numerator", 1),
                  json_interval.value("denominator", 1),
                  json_interval.value("octave", 0));
}

[[nodiscard]] auto rational_is_default(const Rational &rational) -> bool {
  return rational.numerator == 1 && rational.denominator == 1;
}

[[nodiscard]] auto
rational_to_json(const Rational &rational) -> nlohmann::json {
  auto numerator = rational.numerator;
  auto denominator = rational.denominator;

  auto json_rational = nlohmann::json::object();
  if (numerator != 1) {
    json_rational["numerator"] = numerator;
  }
  if (denominator != 1) {
    json_rational["denominator"] = denominator;
  }
  return json_rational;
}

[[nodiscard]] auto
json_to_rational(const nlohmann::json &json_rational) -> Rational {
  return Rational(json_rational.value("numerator", 1),
                  json_rational.value("denominator", 1));
}

void NoteChord::replace_cell(const NoteChord &new_note_chord,
                             NoteChordColumn note_chord_column) {
  switch (note_chord_column) {
  case instrument_column:
    instrument_pointer = new_note_chord.instrument_pointer;
    break;
  case interval_column:
    interval = new_note_chord.interval;
    break;
  case beats_column:
    beats = new_note_chord.beats;
    break;
  case velocity_ratio_column:
    velocity_ratio = new_note_chord.velocity_ratio;
    break;
  case tempo_ratio_column:
    tempo_ratio = new_note_chord.tempo_ratio;
    break;
  case words_column:
    words = new_note_chord.words;
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

NoteChord::NoteChord() : instrument_pointer(get_instrument_pointer("")) {}

NoteChord::NoteChord(const nlohmann::json &json_note_chord)
    : instrument_pointer(
          get_instrument_pointer(json_note_chord.value("instrument", ""))),
      interval(json_note_chord.contains("interval")
                   ? json_to_interval(json_note_chord["interval"])
                   : Interval()),
      beats(json_note_chord.contains("beats")
                ? json_to_rational(json_note_chord["beats"])
                : Rational()),
      velocity_ratio(json_note_chord.contains("velocity_ratio")
                         ? json_to_rational(json_note_chord["velocity_ratio"])
                         : Rational()),
      tempo_ratio(json_note_chord.contains("tempo_ratio")
                      ? json_to_rational(json_note_chord["tempo_ratio"])
                      : Rational()),
      words(QString::fromStdString(json_note_chord.value("words", ""))) {}

// override for chord
auto NoteChord::is_chord() const -> bool { return false; }

// override for chord
auto NoteChord::get_symbol() const -> QString { return "♪"; }

auto NoteChord::to_json() const -> nlohmann::json {
  auto json_note_chord = nlohmann::json::object();
  if (interval.numerator != 1 || interval.denominator != 1 ||
      interval.octave != 0) {
    auto numerator = interval.numerator;
    auto denominator = interval.denominator;
    auto octave = interval.octave;

    auto json_interval = nlohmann::json::object();
    if (numerator != 1) {
      json_interval["numerator"] = numerator;
    }
    if (denominator != 1) {
      json_interval["denominator"] = denominator;
    }
    if (octave != 0) {
      json_interval["octave"] = octave;
    }
    json_note_chord["interval"] = json_interval;
  }
  if (!(rational_is_default(beats))) {
    json_note_chord["beats"] = rational_to_json(beats);
  }
  if (!(rational_is_default(velocity_ratio))) {
    json_note_chord["velocity_ratio"] = rational_to_json(velocity_ratio);
  }
  if (!(rational_is_default(tempo_ratio))) {
    json_note_chord["tempo_ratio"] = rational_to_json(tempo_ratio);
  }
  if (!words.isEmpty()) {
    json_note_chord["words"] = words.toStdString().c_str();
  }
  Q_ASSERT(instrument_pointer != nullptr);
  if (!instrument_is_default(*instrument_pointer)) {
    json_note_chord["instrument"] = instrument_pointer->instrument_name;
  }
  return json_note_chord;
}

auto NoteChord::data(NoteChordColumn note_chord_column) const -> QVariant {
  switch (note_chord_column) {
  case type_column:
    return get_symbol();
  case instrument_column:
    return QVariant::fromValue(instrument_pointer);
  case interval_column:
    return QVariant::fromValue(interval);
  case (beats_column):
    return QVariant::fromValue(beats);
  case velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case tempo_ratio_column:
    return QVariant::fromValue(tempo_ratio);
  case words_column:
    return words;
  default:
    Q_ASSERT(false);
    return {};
  }
};

void NoteChord::setData(NoteChordColumn note_chord_column,
                        const QVariant &new_value) {
  switch (note_chord_column) {
  case instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    instrument_pointer = new_value.value<const Instrument *>();
    break;
  case interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    interval = new_value.value<Interval>();
    break;
  case beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    beats = new_value.value<Rational>();
    break;
  case velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    velocity_ratio = new_value.value<Rational>();
    break;
  case tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    tempo_ratio = new_value.value<Rational>();
    break;
  case words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    words = new_value.toString();
    break;
  default:
    Q_ASSERT(false);
  }
};

void NoteChord::replace_cells(const NoteChord &new_note_chord,
                              NoteChordColumn left_column,
                              NoteChordColumn right_column) {
  for (auto note_chord_column = left_column;
       note_chord_column <= right_column;
       note_chord_column =
           static_cast<NoteChordColumn>(note_chord_column + 1)) {
    replace_cell(new_note_chord, note_chord_column);
  }
}
