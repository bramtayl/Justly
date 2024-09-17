#include "percussion/Percussion.hpp"

#include <QList>
#include <QString>
#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "justly/PercussionColumn.hpp"
#include "other/other.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

auto get_percussion_column_schema(const char *description) -> nlohmann::json {
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
                  {{"percussion_set",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the percussion set"},
                         {"enum", get_names(get_all_percussion_sets())}})},
                   {"percussion_instrument",
                    nlohmann::json(
                        {{"type", "string"},
                         {"description", "the percussion instrument"},
                         {"enum", get_names(get_all_percussion_sets())}})},
                   {"beats", get_rational_schema("the number of beats")},
                   {"velocity_percent", get_rational_schema("velocity ratio")},
                   {"tempo_percent",
                    get_rational_schema("tempo ratio")}})}})}});
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
        json_percussion["percussion_set"] =
            percussion_set_pointer->name.toStdString();

        const auto *percussion_instrument_pointer =
            percussion.percussion_instrument_pointer;
        Q_ASSERT(percussion_instrument_pointer != nullptr);
        json_percussion["percussion_instrument"] =
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
        if (json_percussion.contains("percussion_set")) {
          const auto &percussion_set_value = json_percussion["percussion_set"];
          Q_ASSERT(percussion_set_value.is_string());
          percussion.percussion_set_pointer = get_percussion_set_pointer(
              QString::fromStdString(percussion_set_value.get<std::string>()));
        }
        if (json_percussion.contains("percussion_instrument")) {
          const auto &percussion_value =
              json_percussion["percussion_instrument"];
          Q_ASSERT(percussion_value.is_string());
          percussion.percussion_instrument_pointer =
              get_percussion_instrument_pointer(
                  QString::fromStdString(percussion_value.get<std::string>()));
        }
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