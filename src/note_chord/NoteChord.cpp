#include "note_chord/NoteChord.hpp"

#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

[[nodiscard]] static auto
json_to_interval(const nlohmann::json &json_interval) -> Interval {
  return Interval({json_interval.value("numerator", 1),
                   json_interval.value("denominator", 1),
                   json_interval.value("octave", 0)});
}

[[nodiscard]] static auto
rational_is_default(const Rational &rational) -> bool {
  return rational.numerator == 1 && rational.denominator == 1;
}

[[nodiscard]] static auto
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

[[nodiscard]] static auto
json_to_rational(const nlohmann::json &json_rational) -> Rational {
  return Rational({json_rational.value("numerator", 1),
                   json_rational.value("denominator", 1)});
}

NoteChord::NoteChord(const nlohmann::json &json_note_chord) {
  if (json_note_chord.contains("instrument")) {
    const auto& instrument_value = json_note_chord["instrument"];
    Q_ASSERT(instrument_value.is_string());
    instrument_pointer = get_instrument_pointer(instrument_value.get<std::string>());
  }
  if (json_note_chord.contains("interval")) {
    interval_or_percussion_pointer = json_to_interval(json_note_chord["interval"]);
  }
  if (json_note_chord.contains("percussion")) {
    const auto& percussion_value = json_note_chord["percussion"];
    Q_ASSERT(percussion_value.is_string());
    interval_or_percussion_pointer = get_percussion_pointer(percussion_value.get<std::string>());
  }
  if (json_note_chord.contains("beats")) {
    beats = json_to_rational(json_note_chord["beats"]);
  }
  if (json_note_chord.contains("velocity_ratio")) {
    velocity_ratio = json_to_rational(json_note_chord["velocity_ratio"]);
  }
  if (json_note_chord.contains("tempo_ratio")) {
    tempo_ratio = json_to_rational(json_note_chord["tempo_ratio"]);
  }
  if (json_note_chord.contains("words")) {
    words = QString::fromStdString(json_note_chord.value("words", ""));
  }
}

// override for chord
auto NoteChord::is_chord() const -> bool { return false; }

auto note_chord_to_json(const NoteChord *note_chord_pointer) -> nlohmann::json {
  Q_ASSERT(note_chord_pointer != nullptr);
  auto json_note_chord = nlohmann::json::object();
  const auto & interval_or_percussion_pointer = note_chord_pointer->interval_or_percussion_pointer;
  if (std::holds_alternative<const Percussion* >(interval_or_percussion_pointer)) {
    json_note_chord["percussion"] = std::get<const Percussion*>(interval_or_percussion_pointer)->name;
  } else {
    const auto &interval = std::get<Interval>(interval_or_percussion_pointer);
    auto numerator = interval.numerator;
    auto denominator = interval.denominator;
    auto octave = interval.octave;
    if (numerator != 1 || denominator != 1 || octave != 0) {
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
  }

  const auto &beats = note_chord_pointer->beats;
  if (!(rational_is_default(beats))) {
    json_note_chord["beats"] = rational_to_json(beats);
  }
  const auto &velocity_ratio = note_chord_pointer->velocity_ratio;
  if (!(rational_is_default(velocity_ratio))) {
    json_note_chord["velocity_ratio"] = rational_to_json(velocity_ratio);
  }
  const auto &tempo_ratio = note_chord_pointer->tempo_ratio;
  if (!(rational_is_default(tempo_ratio))) {
    json_note_chord["tempo_ratio"] = rational_to_json(tempo_ratio);
  }
  const auto &words = note_chord_pointer->words;
  if (!words.isEmpty()) {
    json_note_chord["words"] = words.toStdString().c_str();
  }
  const auto *instrument_pointer = note_chord_pointer->instrument_pointer;
  if (instrument_pointer != nullptr && !instrument_is_default(*instrument_pointer)) {
    json_note_chord["instrument"] = instrument_pointer->name;
  }
  return json_note_chord;
}
