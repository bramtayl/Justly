#include "row/chord/Chord.hpp"

#include <QString>
#include <QtGlobal>
#include <concepts>
#include <nlohmann/json.hpp>

#include "abstract_rational/AbstractRational.hpp"
#include "abstract_rational/interval/Interval.hpp"
#include "abstract_rational/rational/Rational.hpp"
#include "justly/ChordColumn.hpp"
#include "named/percussion_instrument/PercussionInstrument.hpp"
#include "named/program/instrument/Instrument.hpp"
#include "named/program/percussion_set/PercussionSet.hpp"
#include "other/other.hpp"
#include "row/note/pitched_note/PitchedNote.hpp"
#include "row/note/unpitched_note/UnpitchedNote.hpp"

template <std::derived_from<Row> SubRow>
static auto json_field_to_rows(nlohmann::json json_object) -> QList<SubRow> {
  const char* field = SubRow::get_plural_field_for();
  if (json_object.contains(field)) {
    QList<SubRow> rows;
    const auto &json_rows = json_object[field];
    json_to_rows(rows, json_rows);
    return rows;
  }
  return {};
}

Chord::Chord(const nlohmann::json &json_chord)
    : Row(json_chord),
      instrument_pointer(json_field_to_named_pointer<Instrument>(json_chord)),
      percussion_set_pointer(
          json_field_to_named_pointer<PercussionSet>(json_chord)),
      percussion_instrument_pointer(
          json_field_to_named_pointer<PercussionInstrument>(json_chord)),
      interval(
          json_field_to_interval(json_chord)),
      tempo_ratio(
          json_field_to_abstract_rational<Rational>(json_chord, "tempo_ratio")),
      pitched_notes(
          json_field_to_rows<PitchedNote>(json_chord)),
      unpitched_notes(
          json_field_to_rows<UnpitchedNote>(json_chord)) {}

auto Chord::get_number_of_columns() -> int { return number_of_chord_columns; }

auto Chord::get_fields_schema() -> nlohmann::json {
  auto schema = Row::get_fields_schema();
  add_pitched_fields_to_schema(schema);
  add_unpitched_fields_to_schema(schema);
  schema["tempo_ratio"] = get_object_schema(get_rational_fields_schema());
  add_row_array_schema<PitchedNote>(schema);
  add_row_array_schema<UnpitchedNote>(schema);
  return schema;
}

auto Chord::get_plural_field_for() -> const char * { return "chords"; }

auto Chord::get_column_name(int column_number) -> const char * {
  switch (column_number) {
  case chord_instrument_column:
    return "Instrument";
  case chord_percussion_set_column:
    return "Percussion set";
  case chord_percussion_instrument_column:
    return "Percussion instrument";
  case chord_interval_column:
    return "Interval";
  case chord_beats_column:
    return "Beats";
  case chord_velocity_ratio_column:
    return "Velocity ratio";
  case chord_tempo_ratio_column:
    return "Tempo ratio";
  case chord_words_column:
    return "Words";
  case chord_pitched_notes_column:
    return "Pitched notes";
  case chord_unpitched_notes_column:
    return "Unpitched notes";
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
  auto json_chord = Row::to_json();
  add_pitched_fields_to_json(json_chord, *this);
  add_unpitched_fields_to_json(json_chord, *this);
  add_abstract_rational_to_json(json_chord, tempo_ratio, "tempo_ratio");
  add_rows_to_json(json_chord, pitched_notes);
  add_rows_to_json(json_chord, unpitched_notes);
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
      add_named_to_json(json_chord, instrument_pointer);
      break;
    case chord_percussion_set_column:
      add_named_to_json(json_chord, percussion_set_pointer);
      break;
    case chord_percussion_instrument_column:
      add_named_to_json(json_chord, percussion_instrument_pointer);
      break;
    case chord_interval_column:
      add_interval_to_json(json_chord, interval);
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
      add_rows_to_json(json_chord, pitched_notes);
      break;
    case chord_unpitched_notes_column:
      add_rows_to_json(json_chord, unpitched_notes);
      break;
    default:
      Q_ASSERT(false);
    }
  }
  return json_chord;
}
