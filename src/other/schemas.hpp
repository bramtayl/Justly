#pragma once

#include "nlohmann/json-schema.hpp"
#include <nlohmann/json.hpp>
#include <string>

[[nodiscard]] auto get_instrument_schema() -> nlohmann::json &;

[[nodiscard]] auto get_note_chord_columns_schema() -> const nlohmann::json &;

[[nodiscard]] auto get_notes_schema() -> const nlohmann::json &;
[[nodiscard]] auto get_chords_schema() -> const nlohmann::json &;

[[nodiscard]] auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;
