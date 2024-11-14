#include "row/note/pitched_note/PitchedNote.hpp"

#include <QString>
#include <QtGlobal>
#include <cmath>
#include <fluidsynth.h>
#include <nlohmann/json.hpp>

#include "abstract_rational/AbstractRational.hpp"
#include "abstract_rational/interval/Interval.hpp"
#include "abstract_rational/rational/Rational.hpp"
#include "justly/PitchedNoteColumn.hpp"
#include "named/program/instrument/Instrument.hpp"
#include "other/other.hpp"
#include "row/Row.hpp"
#include "song/Player.hpp"

struct Program;

static const auto ZERO_BEND_HALFSTEPS = 2;
static const auto BEND_PER_HALFSTEP = 4096;

PitchedNote::PitchedNote(const nlohmann::json &json_note)
    : Note(json_note),
      instrument_pointer(json_field_to_named_pointer<Instrument>(json_note)),
      interval(
          json_field_to_interval(json_note)) {}

auto PitchedNote::get_closest_midi(Player &player, int channel_number,
                                   int /*chord_number*/,
                                   int /*note_number*/) const -> short {
  auto midi_float = get_midi(player.current_key * interval.to_double());
  auto closest_midi = static_cast<short>(round(midi_float));

  fluid_event_pitch_bend(
      player.event_pointer, channel_number,
      to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
             BEND_PER_HALFSTEP));
  send_event_at(player, player.current_time);
  return closest_midi;
}

auto PitchedNote::get_program(const Player &player, int chord_number,
                              int note_number) const -> const Program & {
  return substitute_named_for<PitchedNote>(player.parent, instrument_pointer,
                              player.current_instrument_pointer, "Marimba", chord_number,
                              note_number,
                              "Instrument error", "No instrument",
                              ". Using Marimba."
                              );
}

auto PitchedNote::get_fields_schema() -> nlohmann::json {
  auto schema = Row::get_fields_schema();
  add_pitched_fields_to_schema(schema);
  return schema;
}

auto PitchedNote::get_plural_field_for() -> const char * {
  return "pitched_notes";
}

auto PitchedNote::get_note_type() -> const char * {
  return ", pitched note ";
}

auto PitchedNote::get_column_name(int column_number) -> const char * {
  switch (column_number) {
  case pitched_note_instrument_column:
    return "Instrument";
  case pitched_note_interval_column:
    return "Interval";
  case pitched_note_beats_column:
    return "Beats";
  case pitched_note_velocity_ratio_column:
    return "Velocity ratio";
  case pitched_note_words_column:
    return "Words";
  default:
    Q_ASSERT(false);
    return "";
  }
}

auto PitchedNote::get_number_of_columns() -> int {
  return number_of_pitched_note_columns;
}

auto PitchedNote::get_data(int column_number) const -> QVariant {
  switch (column_number) {
  case pitched_note_instrument_column:
    return QVariant::fromValue(instrument_pointer);
  case pitched_note_interval_column:
    return QVariant::fromValue(interval);
  case pitched_note_beats_column:
    return QVariant::fromValue(beats);
  case pitched_note_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case pitched_note_words_column:
    return words;
  default:
    Q_ASSERT(false);
    return {};
  }
}

void PitchedNote::set_data(int column, const QVariant &new_value) {
  switch (column) {
  case pitched_note_instrument_column:
    instrument_pointer = variant_to<const Instrument *>(new_value);
    break;
  case pitched_note_interval_column:
    interval = variant_to<Interval>(new_value);
    break;
  case pitched_note_beats_column:
    beats = variant_to<Rational>(new_value);
    break;
  case pitched_note_velocity_ratio_column:
    velocity_ratio = variant_to<Rational>(new_value);
    break;
  case pitched_note_words_column:
    words = variant_to<QString>(new_value);
    break;
  default:
    Q_ASSERT(false);
  }
};

void PitchedNote::copy_columns_from(const PitchedNote &template_row,
                                    int left_column, int right_column) {
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (note_column) {
    case pitched_note_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case pitched_note_interval_column:
      interval = template_row.interval;
      break;
    case pitched_note_beats_column:
      beats = template_row.beats;
      break;
    case pitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case pitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

[[nodiscard]] auto PitchedNote::to_json() const -> nlohmann::json {
  auto json_note = Row::to_json();
  add_pitched_fields_to_json(json_note, *this);
  return json_note;
}

[[nodiscard]] auto
PitchedNote::columns_to_json(int left_column,
                             int right_column) const -> nlohmann::json {
  auto json_note = nlohmann::json::object();
  for (auto note_column = left_column; note_column <= right_column;
       note_column++) {
    switch (note_column) {
    case pitched_note_instrument_column:
      add_named_to_json(json_note, instrument_pointer);
      break;
    case pitched_note_interval_column:
      add_interval_to_json(json_note, interval);
      break;
    case pitched_note_beats_column:
      add_abstract_rational_to_json(json_note, beats, "beats");
      break;
    case pitched_note_velocity_ratio_column:
      add_abstract_rational_to_json(json_note, velocity_ratio,
                                    "velocity_ratio");
      break;
    case pitched_note_words_column:
      add_words_to_json(json_note, words);
      break;
    default:
      Q_ASSERT(false);
      break;
    }
  }
  return json_note;
}
