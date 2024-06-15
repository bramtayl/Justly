#include <nlohmann/json_fwd.hpp>  // for json

[[nodiscard]] auto note_schema() -> const nlohmann::json &;
[[nodiscard]] auto chord_schema() -> const nlohmann::json &;
[[nodiscard]] auto verify_json_song(const nlohmann::json &) -> bool;