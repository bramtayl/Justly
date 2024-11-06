#include "chord/Chord.hpp"

#include <QObject>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "named/Named.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "pitched_note/PitchedNote.hpp"
#include "rational/AbstractRational.hpp"
#include "rational/Rational.hpp"
#include "unpitched_note/UnpitchedNote.hpp"

Chord::Chord(const nlohmann::json &json_chord)
    : instrument_pointer(
          json_field_to_named_pointer<Instrument>(json_chord, "instrument")),
      percussion_set_pointer(json_field_to_named_pointer<PercussionSet>(
          json_chord, "percussion_set")),
      percussion_instrument_pointer(
          json_field_to_named_pointer<PercussionInstrument>(
              json_chord, "percussion_instrument")),
      interval(
          json_field_to_abstract_rational<Interval>(json_chord, "interval")),
      beats(json_field_to_abstract_rational<Rational>(json_chord, "beats")),
      velocity_ratio(json_field_to_abstract_rational<Rational>(
          json_chord, "velocity_ratio")),
      tempo_ratio(
          json_field_to_abstract_rational<Rational>(json_chord, "tempo_ratio")),
      words(json_field_to_words(json_chord)),
      pitched_notes(
          json_field_to_rows<PitchedNote>(json_chord, "pitched_notes")),
      unpitched_notes(
          json_field_to_rows<UnpitchedNote>(json_chord, "unpitched_notes")) {}

auto Chord::get_number_of_columns() -> int { return number_of_chord_columns; }

auto Chord::get_column_name(int column_number) -> QString {
  switch (column_number) {
  case chord_instrument_column:
    return QObject::tr("Instrument");
  case chord_percussion_set_column:
    return QObject::tr("Percussion set");
  case chord_percussion_instrument_column:
    return QObject::tr("Percussion instrument");
  case chord_interval_column:
    return QObject::tr("Interval");
  case chord_beats_column:
    return QObject::tr("Beats");
  case chord_velocity_ratio_column:
    return QObject::tr("Velocity ratio");
  case chord_tempo_ratio_column:
    return QObject::tr("Tempo ratio");
  case chord_words_column:
    return QObject::tr("Words");
  case chord_pitched_notes_column:
    return QObject::tr("Pitched notes");
  case chord_unpitched_notes_column:
    return QObject::tr("Unpitched notes");
  default:
    Q_ASSERT(false);
    return "";
  }
}

auto Chord::is_column_editable(int column_number) -> bool {
  return column_number != chord_pitched_notes_column &&
         column_number != chord_unpitched_notes_column;
}

auto Chord::get_data(int column_number) const -> QVariant {
  switch (column_number) {
  case chord_instrument_column:
    return QVariant::fromValue(instrument_pointer);
  case chord_percussion_set_column:
    return QVariant::fromValue(percussion_set_pointer);
  case chord_percussion_instrument_column:
    return QVariant::fromValue(percussion_instrument_pointer);
  case chord_interval_column:
    return QVariant::fromValue(interval);
  case chord_beats_column:
    return QVariant::fromValue(beats);
  case chord_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case chord_tempo_ratio_column:
    return QVariant::fromValue(tempo_ratio);
  case chord_words_column:
    return words;
  case chord_pitched_notes_column:
    return pitched_notes.size();
  case chord_unpitched_notes_column:
    return unpitched_notes.size();
  default:
    Q_ASSERT(false);
    return {};
  }
}

void Chord::set_data(int column, const QVariant &new_value) {
  switch (column) {
  case chord_instrument_column:
    instrument_pointer = variant_to<const Instrument *>(new_value);
    break;
  case chord_percussion_set_column:
    percussion_set_pointer = variant_to<const PercussionSet *>(new_value);
    break;
  case chord_percussion_instrument_column:
    percussion_instrument_pointer =
        variant_to<const PercussionInstrument *>(new_value);
    break;
  case chord_interval_column:
    interval = variant_to<Interval>(new_value);
    break;
  case chord_beats_column:
    beats = variant_to<Rational>(new_value);
    break;
  case chord_velocity_ratio_column:
    velocity_ratio = variant_to<Rational>(new_value);
    break;
  case chord_tempo_ratio_column:
    tempo_ratio = variant_to<Rational>(new_value);
    break;
  case chord_words_column:
    words = variant_to<QString>(new_value);
    break;
  default:
    Q_ASSERT(false);
  }
};

void Chord::copy_columns_from(const Chord &template_row, int left_column,
                              int right_column) {
  for (auto chord_column = left_column; chord_column <= right_column;
       chord_column++) {
    switch (chord_column) {
    case chord_instrument_column:
      instrument_pointer = template_row.instrument_pointer;
      break;
    case chord_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case chord_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
      break;
    case chord_interval_column:
      interval = template_row.interval;
      break;
    case chord_beats_column:
      beats = template_row.beats;
      break;
    case chord_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case chord_tempo_ratio_column:
      tempo_ratio = template_row.tempo_ratio;
      break;
    case chord_words_column:
      words = template_row.words;
      break;
    case chord_pitched_notes_column:
      pitched_notes = template_row.pitched_notes;
      break;
    case chord_unpitched_notes_column:
      unpitched_notes = template_row.unpitched_notes;
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

[[nodiscard]] auto Chord::to_json() const -> nlohmann::json {
  auto json_chord = nlohmann::json::object();

  add_named_to_json(json_chord, instrument_pointer, "instrument");
  add_named_to_json(json_chord, percussion_set_pointer, "percussion_set");
  add_named_to_json(json_chord, percussion_instrument_pointer,
                    "percussion_instrument");
  add_abstract_rational_to_json(json_chord, interval, "interval");
  add_abstract_rational_to_json(json_chord, beats, "beats");
  add_abstract_rational_to_json(json_chord, velocity_ratio, "velocity_ratio");
  add_abstract_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
  add_words_to_json(json_chord, words);
  add_rows_to_json(json_chord, pitched_notes, "pitched_notes");
  add_rows_to_json(json_chord, unpitched_notes, "unpitched_notes");
  return json_chord;
}

[[nodiscard]] auto
Chord::columns_to_json(int left_column,
                       int right_column) const -> nlohmann::json {
  auto json_chord = nlohmann::json::object();

  for (auto chord_column = left_column; chord_column <= right_column;
       chord_column = chord_column + 1) {
    switch (chord_column) {
    case chord_instrument_column:
      add_named_to_json(json_chord, instrument_pointer, "instrument");
      break;
    case chord_percussion_set_column:
      add_named_to_json(json_chord, percussion_set_pointer, "percussion_set");
      break;
    case chord_percussion_instrument_column:
      add_named_to_json(json_chord, percussion_instrument_pointer,
                        "percussion_instrument");
      break;
    case chord_interval_column:
      add_abstract_rational_to_json(json_chord, interval, "interval");
      break;
    case chord_beats_column:
      add_abstract_rational_to_json(json_chord, beats, "beats");
      break;
    case chord_velocity_ratio_column:
      add_abstract_rational_to_json(json_chord, velocity_ratio,
                                    "velocity_ratio");
      break;
    case chord_tempo_ratio_column:
      add_abstract_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
      break;
    case chord_words_column:
      add_words_to_json(json_chord, words);
      break;
    case chord_pitched_notes_column:
      add_rows_to_json(json_chord, pitched_notes, "pitched_notes");
      break;
    case chord_unpitched_notes_column:
      add_rows_to_json(json_chord, unpitched_notes, "unpitched_notes");
      break;
    default:
      Q_ASSERT(false);
    }
  }
  return json_chord;
}

auto get_chords_schema() -> nlohmann::json {
  return get_array_schema(
      "a list of chords",
      get_object_schema(
          "a chord",
          nlohmann::json(
              {{"instrument", get_named_schema<Instrument>("the instrument")},
               {"percussion_set",
                get_named_schema<PercussionSet>("the percussion set")},
               {"percussion_instrument", get_named_schema<PercussionInstrument>(
                                             "the percussion instrument")},
               {"interval", get_interval_schema()},
               {"beats", get_rational_schema("the number of beats")},
               {"velocity_percent", get_rational_schema("velocity ratio")},
               {"tempo_percent", get_rational_schema("tempo ratio")},
               {"words", get_words_schema()},
               {"pitched_notes", get_pitched_notes_schema()},
               {"unpitched_notes", get_unpitched_notes_schema()}})));
}
