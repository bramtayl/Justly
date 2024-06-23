#include "justly/NoteChord.hpp"

#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string, allocator, opera...

#include "justly/Instrument.hpp"  // for get_instrument, Instrument
#include "justly/Interval.hpp"    // for Interval
#include "justly/Rational.hpp"

NoteChord::NoteChord()
    : beats(DEFAULT_BEATS),
      words(DEFAULT_WORDS),
      instrument_pointer(&(get_instrument(""))) {}

NoteChord::NoteChord(const nlohmann::json& json_note_chord)
    : interval(json_note_chord.contains("interval")
                   ? Interval(json_note_chord["interval"])
                   : Interval()),
      beats(json_note_chord.value("beats", DEFAULT_BEATS)),
      volume_ratio(json_note_chord.contains("volume_ratio")
                   ? Rational(json_note_chord["volume_ratio"])
                   : Rational()),
      tempo_ratio(json_note_chord.contains("tempo_ratio")
                   ? Rational(json_note_chord["tempo_ratio"])
                   : Rational()),
      words(json_note_chord.value("words", DEFAULT_WORDS)),
      instrument_pointer(
          &get_instrument(json_note_chord.value("instrument", ""))) {}

auto NoteChord::json() const -> nlohmann::json {
  auto json_note_chord = nlohmann::json::object();
  if (!(interval.is_default())) {
    json_note_chord["interval"] = interval.json();
  }
  if (beats != DEFAULT_BEATS) {
    json_note_chord["beats"] = beats;
  }
  if (!(volume_ratio.is_default())) {
    json_note_chord["volume_ratio"] = volume_ratio.json();
  }
  if (!(tempo_ratio.is_default())) {
    json_note_chord["tempo_ratio"] = tempo_ratio.json();
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
