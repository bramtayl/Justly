#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/NoteChord.h"  // for NoteChord

struct Note : NoteChord {
  Note() = default;
  explicit Note(const nlohmann::json &);
  ~Note() override = default;
  [[nodiscard]] auto symbol() const -> std::string override;
  [[nodiscard]] static auto json_schema() -> const nlohmann::json &;
};
