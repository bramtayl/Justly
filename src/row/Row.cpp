#include "row/Row.hpp"

#include "abstract_rational/AbstractRational.hpp"
#include "other/other.hpp"

static auto json_field_to_words(const nlohmann::json &json_row) -> QString {
  if (json_row.contains("words")) {
    return QString::fromStdString(json_row["words"]);
  }
  return "";
}

Row::Row(const nlohmann::json &json_chord)
    : beats(json_field_to_abstract_rational<Rational>(json_chord, "beats")),
      velocity_ratio(json_field_to_abstract_rational<Rational>(
          json_chord, "velocity_ratio")),
      words(json_field_to_words(json_chord)) {}

[[nodiscard]] auto Row::to_json() const -> nlohmann::json {
  auto json_row = nlohmann::json::object();
  add_abstract_rational_to_json(json_row, beats, "beats");
  add_abstract_rational_to_json(json_row, velocity_ratio,
                                "velocity_ratio");
  add_words_to_json(json_row, words);
  return json_row;
}

auto Row::is_column_editable(int /*column_number*/) -> bool {
    return true;
};