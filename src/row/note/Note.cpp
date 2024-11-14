#include "row/note/Note.hpp"

#include <nlohmann/json.hpp>

Note::Note(const nlohmann::json &json_note) : Row(json_note) {}

