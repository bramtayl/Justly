#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>                 // for vector

[[nodiscard]] auto note_schema() -> const nlohmann::json &;
[[nodiscard]] auto chord_schema() -> const nlohmann::json &;
[[nodiscard]] auto song_schema() -> const nlohmann::json &;
[[nodiscard]] auto instrument_names() -> const std::vector<std::string> &;