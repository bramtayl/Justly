#pragma once

#include <nlohmann/json.hpp>  // for json
#include <string>

#include "justly/NoteChord.hpp"  // for NoteChord
#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Note : NoteChord {
  Note() = default;
  explicit Note(const nlohmann::json& json_note);
  ~Note() override = default;

  [[nodiscard]] auto symbol() const -> std::string override;
};

[[nodiscard]] auto get_note_schema() -> const nlohmann::json&;
