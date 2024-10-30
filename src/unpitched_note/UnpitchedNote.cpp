#include "unpitched_note/UnpitchedNote.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "justly/UnpitchedNoteColumn.hpp"
#include "named/Named.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"
#include "rows/json_field_conversions.hpp"

auto to_percussion_column(int column) -> UnpitchedNoteColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_UNPITCHED_NOTE_COLUMNS);
  return static_cast<UnpitchedNoteColumn>(column);
}

void UnpitchedNote::from_json(const nlohmann::json &json_percussion) {
  percussion_set_pointer_from_json(*this, json_percussion);
  percussion_instrument_pointer_from_json(*this, json_percussion);
  beats_from_json(*this, json_percussion);
  velocity_ratio_from_json(*this, json_percussion);
}

auto UnpitchedNote::get_data(int column_number) const -> QVariant {
  switch (to_percussion_column(column_number)) {
  case unpitched_note_percussion_set_column:
    return QVariant::fromValue(percussion_set_pointer);
  case unpitched_note_percussion_instrument_column:
    return QVariant::fromValue(percussion_instrument_pointer);
  case unpitched_note_beats_column:
    return QVariant::fromValue(beats);
  case unpitched_note_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  }
}

void UnpitchedNote::set_data_directly(int column, const QVariant &new_value) {
  switch (to_percussion_column(column)) {
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
  }
};

void UnpitchedNote::copy_columns_from(const UnpitchedNote &template_row,
                                      int left_column, int right_column) {
  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (to_percussion_column(percussion_column)) {
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
    }
  }
};

[[nodiscard]] auto
UnpitchedNote::to_json(int left_column,
                       int right_column) const -> nlohmann::json {
  auto json_percussion = nlohmann::json::object();

  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (to_percussion_column(percussion_column)) {
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
    }
  }
  return json_percussion;
}

[[nodiscard]] static auto
get_unpitched_note_column_schema(const char *description) {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", unpitched_note_percussion_set_column},
                         {"maximum", unpitched_note_velocity_ratio_column}});
}

auto get_unpitched_notes_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "the unpitched_notes"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a unpitched_note"},
             {"properties",
              nlohmann::json(
                  {{"percussion_set", get_percussion_set_schema()},
                   {"percussion_instrument",
                    get_percussion_instrument_schema()},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_ratio",
                    get_rational_schema("velocity ratio")}})}})}});
}

auto get_unpitched_notes_cells_validator()
    -> const nlohmann::json_schema::json_validator & {
  static const auto
      unpitched_notes_cells_validator = make_validator(
          "Percussions cells",
          nlohmann::json(
              {{"description", "cells"},
               {"type", "object"},
               {"required", {"left_column", "right_column", "unpitched_notes"}},
               {"properties",
                nlohmann::json(
                    {{"left_column", get_unpitched_note_column_schema(
                                         "left unpitched_note column")},
                     {"right_column", get_unpitched_note_column_schema(
                                          "right unpitched_note column")},
                     {"unpitched_notes", get_unpitched_notes_schema()}})}}));
  return unpitched_notes_cells_validator;
}
