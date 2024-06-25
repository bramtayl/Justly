#pragma once

#include <nlohmann/json_fwd.hpp>  // for json

[[nodiscard]] auto get_note_schema() -> const nlohmann::json &;
[[nodiscard]] auto get_chord_schema() -> const nlohmann::json &;

[[nodiscard]] auto verify_json_song(const nlohmann::json &json_song) -> bool;