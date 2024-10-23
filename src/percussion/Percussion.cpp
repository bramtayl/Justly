#include "percussion/Percussion.hpp"

#include <QList>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "justly/PercussionColumn.hpp"
#include "other/other.hpp"
#include "percussion/PercussionsModel.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

Percussion::Percussion(const nlohmann::json &json_percussion) {
  if (json_percussion.contains("percussion_set")) {
    percussion_set_pointer = &json_to_item(get_all_percussion_sets(),
                                           json_percussion["percussion_set"]);
  }
  if (json_percussion.contains("percussion_instrument")) {
    percussion_instrument_pointer =
        &json_to_item(get_all_percussion_instruments(),
                      json_percussion["percussion_instrument"]);
  }
  if (json_percussion.contains("beats")) {
    beats = json_to_rational(json_percussion["beats"]);
  }
  if (json_percussion.contains("velocity_ratio")) {
    velocity_ratio = json_to_rational(json_percussion["velocity_ratio"]);
  }
  if (json_percussion.contains("tempo_ratio")) {
    tempo_ratio = json_to_rational(json_percussion["tempo_ratio"]);
  }
}

void Percussion::copy_columns_from(const Percussion &template_percussion, int left_column,
                              int right_column) {
  for (auto percussion_column = left_column; percussion_column <= right_column;
       percussion_column++) {
    switch (to_percussion_column(percussion_column)) {
    case percussion_instrument_column:
      percussion_instrument_pointer =
          template_percussion.percussion_instrument_pointer;
      break;
    case percussion_set_column:
      percussion_set_pointer = template_percussion.percussion_set_pointer;
      break;
    case percussion_beats_column:
      beats = template_percussion.beats;
      break;
    case percussion_velocity_ratio_column:
      velocity_ratio = template_percussion.velocity_ratio;
      break;
    case percussion_tempo_ratio_column:
      tempo_ratio = template_percussion.tempo_ratio;
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
    case percussion_instrument_column:
      if (percussion_instrument_pointer != nullptr) {
        json_percussion["percussion_instrument"] =
            item_to_json(*percussion_instrument_pointer);
      }
      break;
    case percussion_set_column:
      if (percussion_set_pointer != nullptr) {
        json_percussion["percussion_set"] =
            item_to_json(*percussion_set_pointer);
      }
      break;
    case percussion_beats_column:
      if (!(rational_is_default(beats))) {
        json_percussion["beats"] = rational_to_json(beats);
      }
      break;
    case percussion_velocity_ratio_column:
      if (!(rational_is_default(velocity_ratio))) {
        json_percussion["velocity_ratio"] = rational_to_json(velocity_ratio);
      }
      break;
    case percussion_tempo_ratio_column:
      if (!(rational_is_default(tempo_ratio))) {
        json_percussion["tempo_ratio"] = rational_to_json(tempo_ratio);
      }
      break;
    }
  }
  return json_percussion;
}

[[nodiscard]] static auto
get_percussion_column_schema(const char *description) -> nlohmann::json {
  return nlohmann::json({{"type", "number"},
                         {"description", description},
                         {"minimum", percussion_set_column},
                         {"maximum", percussion_tempo_ratio_column}});
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
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent",
                    get_rational_schema("tempo ratio")}})}})}});
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
