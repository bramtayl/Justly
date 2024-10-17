#include "percussion/Percussion.hpp"

#include <QList>
#include <QString>
#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "justly/PercussionColumn.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

[[nodiscard]] static auto get_percussion_column_schema(const char *description) -> nlohmann::json {
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
                  {{"set",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the percussion set"},
                         {"enum", get_names(get_all_percussion_sets())}})},
                   {"instrument",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the percussion instrument"},
                         {"enum",
                          get_names(get_all_percussion_instruments())}})},
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

auto percussions_to_json(const QList<Percussion> &percussions,
                         qsizetype first_percussion_number,
                         qsizetype number_of_percussions) -> nlohmann::json {
  nlohmann::json json_percussions = nlohmann::json::array();
  std::transform(
      percussions.cbegin() + static_cast<int>(first_percussion_number),
      percussions.cbegin() +
          static_cast<int>(first_percussion_number + number_of_percussions),
      std::back_inserter(json_percussions),
      [](const Percussion &percussion) -> nlohmann::json {
        auto json_percussion = nlohmann::json::object();

        const auto *percussion_set_pointer = percussion.percussion_set_pointer;
        Q_ASSERT(percussion_set_pointer != nullptr);
        json_percussion["set"] = percussion_set_pointer->name.toStdString();

        const auto *percussion_instrument_pointer =
            percussion.percussion_instrument_pointer;
        Q_ASSERT(percussion_instrument_pointer != nullptr);
        json_percussion["instrument"] =
            percussion_instrument_pointer->name.toStdString();

        const auto &beats = percussion.beats;
        if (!(rational_is_default(beats))) {
          json_percussion["beats"] = rational_to_json(beats);
        }
        const auto &velocity_ratio = percussion.velocity_ratio;
        if (!(rational_is_default(velocity_ratio))) {
          json_percussion["velocity_ratio"] = rational_to_json(velocity_ratio);
        }
        const auto &tempo_ratio = percussion.tempo_ratio;
        if (!(rational_is_default(tempo_ratio))) {
          json_percussion["tempo_ratio"] = rational_to_json(tempo_ratio);
        }
        return json_percussion;
      });
  return json_percussions;
}

void json_to_percussions(QList<Percussion> &new_percussions,
                         const nlohmann::json &json_percussions,
                         qsizetype number_of_percussions) {
  std::transform(
      json_percussions.cbegin(),
      json_percussions.cbegin() + static_cast<int>(number_of_percussions),
      std::back_inserter(new_percussions),
      [](const nlohmann::json &json_percussion) -> Percussion {
        Percussion percussion;
        Q_ASSERT(json_percussion.contains("set"));
        const auto &percussion_set_value = json_percussion["set"];
        Q_ASSERT(percussion_set_value.is_string());
        percussion.percussion_set_pointer = get_percussion_set_pointer(
            QString::fromStdString(percussion_set_value.get<std::string>()));
        Q_ASSERT(json_percussion.contains("instrument"));
        const auto &instrument_value = json_percussion["instrument"];
        Q_ASSERT(instrument_value.is_string());
        percussion.percussion_instrument_pointer =
            get_percussion_instrument_pointer(
                QString::fromStdString(instrument_value.get<std::string>()));
        if (json_percussion.contains("beats")) {
          percussion.beats = json_to_rational(json_percussion["beats"]);
        }
        if (json_percussion.contains("velocity_ratio")) {
          percussion.velocity_ratio =
              json_to_rational(json_percussion["velocity_ratio"]);
        }
        if (json_percussion.contains("tempo_ratio")) {
          percussion.tempo_ratio =
              json_to_rational(json_percussion["tempo_ratio"]);
        }
        return percussion;
      });
}