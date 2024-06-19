#include <nlohmann/json_fwd.hpp>  // for json

#include "justly/global.h"

[[nodiscard]] JUSTLY_EXPORT auto note_schema() -> const nlohmann::json &;
[[nodiscard]] JUSTLY_EXPORT auto chord_schema() -> const nlohmann::json &;
[[nodiscard]] JUSTLY_EXPORT auto verify_json_song(const nlohmann::json &) -> bool;