#include "percussion/Percussion.hpp"

#include <QtGlobal>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "justly/PercussionColumn.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"
#include "rows/json_field_conversions.hpp"

auto to_percussion_column(int column) -> PercussionColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_PERCUSSION_COLUMNS);
  return static_cast<PercussionColumn>(column);
}

void Percussion::from_json(const nlohmann::json &json_percussion) {
  percussion_set_pointer_from_json(*this, json_percussion);
  percussion_instrument_pointer_from_json(*this, json_percussion);
  beats_from_json(*this, json_percussion);
  velocity_ratio_from_json(*this, json_percussion);
}

auto Percussion::get_data(int column_number) const -> QVariant {
  switch (to_percussion_column(column_number)) {
  case percussion_percussion_set_column:
    return QVariant::fromValue(percussion_set_pointer);
  case percussion_percussion_instrument_column:
    return QVariant::fromValue(percussion_instrument_pointer);
  case percussion_beats_column:
    return QVariant::fromValue(beats);
  case percussion_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  }
}

void Percussion::set_data_directly(int column, const QVariant &new_value) {
  switch (to_percussion_column(column)) {
  case percussion_percussion_set_column:
    percussion_set_pointer = variant_to_percussion_set(new_value);
    break;
  case percussion_percussion_instrument_column:
    percussion_instrument_pointer = variant_to_percussion_instrument(new_value);
    break;
  case percussion_beats_column:
    beats = variant_to_rational(new_value);
    break;
  case percussion_velocity_ratio_column:
    velocity_ratio = variant_to_rational(new_value);
    break;
  }
};

void Percussion::copy_columns_from(const Percussion &template_row,
                                   int left_column, int right_column) {
  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (to_percussion_column(percussion_column)) {
    case percussion_percussion_set_column:
      percussion_set_pointer = template_row.percussion_set_pointer;
      break;
    case percussion_percussion_instrument_column:
      percussion_instrument_pointer =
          template_row.percussion_instrument_pointer;
      break;
    case percussion_beats_column:
      beats = template_row.beats;
      break;
    case percussion_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    }
  }
};

[[nodiscard]] auto
Percussion::to_json(int left_column, int right_column) const -> nlohmann::json {
  auto json_percussion = nlohmann::json::object();

  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (to_percussion_column(percussion_column)) {
    case percussion_percussion_set_column:
      add_named_to_json(json_percussion, percussion_set_pointer,
                        "percussion_set");
      break;
    case percussion_percussion_instrument_column:
      add_named_to_json(json_percussion, percussion_instrument_pointer,
                        "percussion_instrument");
      break;
    case percussion_beats_column:
      add_rational_to_json(json_percussion, beats, "beats");
      break;
    case percussion_velocity_ratio_column:
      add_rational_to_json(json_percussion, velocity_ratio, "velocity_ratio");
      break;
    }
  }
  return json_percussion;
}

[[nodiscard]] static auto
get_percussion_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", percussion_percussion_set_column},
                         {"maximum", percussion_velocity_ratio_column}});
}

auto get_percussions_schema() -> nlohmann::json {
  return nlohmann::json(
      {{"type", "array"},
       {"description", "the percussions"},
       {"items",
        nlohmann::json(
            {{"type", "object"},
             {"description", "a percussion"},
             {"properties",
              nlohmann::json(
                  {{"percussion_set", get_percussion_set_schema()},
                   {"percussion_instrument",
                    get_percussion_instrument_schema()},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_ratio",
                    get_rational_schema("velocity ratio")}})}})}});
}

auto get_percussions_cells_validator()
    -> const nlohmann::json_schema::json_validator & {
  static const nlohmann::json_schema::json_validator
      percussions_cells_validator = make_validator(
          "Percussions cells",
          nlohmann::json(
              {{"description", "cells"},
               {"type", "object"},
               {"required", {"left_column", "right_column", "percussions"}},
               {"properties",
                nlohmann::json(
                    {{"left_column",
                      get_percussion_column_schema("left percussion column")},
                     {"right_column",
                      get_percussion_column_schema("right percussion column")},
                     {"percussions", get_percussions_schema()}})}}));
  return percussions_cells_validator;
}
