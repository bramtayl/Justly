#include "percussion/Percussion.hpp"

#include <QtGlobal>
#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>

#include "other/templates.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"

Percussion::Percussion()
    : percussion_set_pointer(get_percussion_set_pointer("Standard")),
      percussion_instrument_pointer(
          get_percussion_instrument_pointer("Tambourine")) {}

auto percussions_to_json(const std::vector<Percussion> &percussions,
                         size_t first_percussion_number,
                         size_t number_of_percussions) -> nlohmann::json {
  nlohmann::json json_percussions;
  check_range(percussions, first_percussion_number, number_of_percussions);
  std::transform(
      percussions.cbegin() + static_cast<int>(first_percussion_number),
      percussions.cbegin() +
          static_cast<int>(first_percussion_number + number_of_percussions),
      std::back_inserter(json_percussions),
      [](const Percussion &percussion) -> nlohmann::json {
        auto json_percussion = nlohmann::json::object();

        const auto *percussion_set_pointer = percussion.percussion_set_pointer;
        Q_ASSERT(percussion_set_pointer != nullptr);
        json_percussion["percussion_set"] = percussion_set_pointer->name;

        const auto *percussion_instrument_pointer =
            percussion.percussion_instrument_pointer;
        Q_ASSERT(percussion_instrument_pointer != nullptr);
        json_percussion["percussion_instrument"] =
            percussion_instrument_pointer->name;

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
        const auto &words = percussion.words;
        if (!words.isEmpty()) {
          json_percussion["words"] = words.toStdString().c_str();
        }
        return json_percussion;
      });
  return json_percussions;
}

void json_to_percussions(std::vector<Percussion> &new_percussions,
                         const nlohmann::json &json_percussions,
                         size_t number_of_percussions) {
  std::transform(
      json_percussions.cbegin(),
      json_percussions.cbegin() + static_cast<int>(number_of_percussions),
      std::back_inserter(new_percussions),
      [](const nlohmann::json &json_percussion) -> Percussion {
        Percussion percussion;
        if (json_percussion.contains("percussion_set")) {
          const auto &percussion_set_value = json_percussion["instrument"];
          Q_ASSERT(percussion_set_value.is_string());
          percussion.percussion_set_pointer = get_percussion_set_pointer(
              percussion_set_value.get<std::string>());
        }
        if (json_percussion.contains("percussion_instrument")) {
          const auto &percussion_value =
              json_percussion["percussion_instrument"];
          Q_ASSERT(percussion_value.is_string());
          percussion.percussion_instrument_pointer =
              get_percussion_instrument_pointer(
                  percussion_value.get<std::string>());
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
        if (json_percussion.contains("words")) {
          percussion.words =
              QString::fromStdString(json_percussion.value("words", ""));
        }
        return percussion;
      });
}
