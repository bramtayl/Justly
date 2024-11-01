#include "unpitched_note/UnpitchedNote.hpp"

#include <QObject>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "justly/UnpitchedNoteColumn.hpp"
#include "named/Named.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

UnpitchedNote::UnpitchedNote(const nlohmann::json &json_note)
    : percussion_set_pointer(json_field_to_named_pointer(
          PercussionSet::get_all_nameds(), json_note, "percussion_set")),
      percussion_instrument_pointer(
          json_field_to_named_pointer(PercussionInstrument::get_all_nameds(),
                                      json_note, "percussion_instrument")),
      beats(json_field_to_rational(json_note, "beats")),
      velocity_ratio(json_field_to_rational(json_note, "velocity_ratio")),
      words(json_field_to_words(json_note)) {}

auto UnpitchedNote::get_number_of_columns() -> int {
  return number_of_unpitched_note_columns;
}

auto UnpitchedNote::get_column_name(int column_number) -> QString {
  switch (column_number) {
  case unpitched_note_percussion_set_column:
    return QObject::tr("Percussion set");
  case unpitched_note_percussion_instrument_column:
    return QObject::tr("Percussion instrument");
  case unpitched_note_beats_column:
    return QObject::tr("Beats");
  case unpitched_note_velocity_ratio_column:
    return QObject::tr("Velocity ratio");
  case unpitched_note_words_column:
    return QObject::tr("Words");
  default:
    Q_ASSERT(false);
  }
}

auto UnpitchedNote::get_data(int column_number) const -> QVariant {
  switch (column_number) {
  case unpitched_note_percussion_set_column:
    return QVariant::fromValue(percussion_set_pointer);
  case unpitched_note_percussion_instrument_column:
    return QVariant::fromValue(percussion_instrument_pointer);
  case unpitched_note_beats_column:
    return QVariant::fromValue(beats);
  case unpitched_note_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case unpitched_note_words_column:
    return words;
  default:
    Q_ASSERT(false);
    return {};
  }
}

void UnpitchedNote::set_data_directly(int column, const QVariant &new_value) {
  switch (column) {
  case unpitched_note_percussion_set_column:
    percussion_set_pointer = variant_to_percussion_set(new_value);
    break;
  case unpitched_note_percussion_instrument_column:
    percussion_instrument_pointer = variant_to_percussion_instrument(new_value);
    break;
  case unpitched_note_beats_column:
    beats = variant_to_rational(new_value);
    break;
  case unpitched_note_velocity_ratio_column:
    velocity_ratio = variant_to_rational(new_value);
    break;
  case unpitched_note_words_column:
    words = variant_to_string(new_value);
    break;
  default:
    Q_ASSERT(false);
  }
};

void UnpitchedNote::copy_columns_from(const UnpitchedNote &template_row,
                                      int left_column, int right_column) {
  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (percussion_column) {
    case unpitched_note_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case unpitched_note_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
      break;
    case unpitched_note_beats_column:
      beats = template_row.beats;
      break;
    case unpitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case unpitched_note_words_column:
      words = template_row.words;
      break;
    default:
      Q_ASSERT(false);
    }
  }
};

[[nodiscard]] auto UnpitchedNote::to_json() const -> nlohmann::json {
  auto json_percussion = nlohmann::json::object();
  add_named_to_json(json_percussion, percussion_set_pointer, "percussion_set");
  add_named_to_json(json_percussion, percussion_instrument_pointer,
                    "percussion_instrument");
  add_rational_to_json(json_percussion, beats, "beats");
  add_rational_to_json(json_percussion, velocity_ratio, "velocity_ratio");
  add_words_to_json(json_percussion, words);
  return json_percussion;
}

[[nodiscard]] auto
UnpitchedNote::columns_to_json(int left_column,
                               int right_column) const -> nlohmann::json {
  auto json_percussion = nlohmann::json::object();

  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (percussion_column) {
    case unpitched_note_percussion_set_column:
      add_named_to_json(json_percussion, percussion_set_pointer,
                        "percussion_set");
      break;
    case unpitched_note_percussion_instrument_column:
      add_named_to_json(json_percussion, percussion_instrument_pointer,
                        "percussion_instrument");
      break;
    case unpitched_note_beats_column:
      add_rational_to_json(json_percussion, beats, "beats");
      break;
    case unpitched_note_velocity_ratio_column:
      add_rational_to_json(json_percussion, velocity_ratio, "velocity_ratio");
      break;
    case unpitched_note_words_column:
      add_words_to_json(json_percussion, words);
      break;
    default:
      Q_ASSERT(false);
    }
  }
  return json_percussion;
}

auto get_unpitched_notes_schema() -> nlohmann::json {
  return get_array_schema(
      "the unpitched_notes",
      get_object_schema(
          "a unpitched_note",
          nlohmann::json(
              {{"percussion_set",
                get_named_schema(PercussionSet::get_all_nameds(),
                                 "the percussion set")},
               {"percussion_instrument",
                get_named_schema(PercussionInstrument::get_all_nameds(),
                                 "the percussion instrument")},
               {"beats", get_rational_schema("the number of beats")},
               {"velocity_ratio", get_rational_schema("velocity ratio")}})));
}
