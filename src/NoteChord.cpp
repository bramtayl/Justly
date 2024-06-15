#include "justly/NoteChord.h"

#include <map>                               // for operator!=, operator==
#include <nlohmann/detail/json_pointer.hpp>  // for json_pointer<>::string_t
#include <nlohmann/json.hpp>                 // for basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>             // for json
#include <string>                            // for string, allocator, opera...

#include "justly/Instrument.h"  // for get_instrument, Instrument
#include "justly/Interval.h"    // for Interval

NoteChord::NoteChord()
    : beats(DEFAULT_BEATS),
      volume_percent(DEFAULT_VOLUME_PERCENT),
      tempo_percent(DEFAULT_TEMPO_PERCENT),
      words(DEFAULT_WORDS),
      instrument_pointer(&(get_instrument(""))) {}

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
