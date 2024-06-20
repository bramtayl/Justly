#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>

#include "justly/global.hpp"
#include "justly/NoteChord.hpp"  // for NoteChord

struct JUSTLY_EXPORT Note : NoteChord {
  Note() = default;
  explicit Note(const nlohmann::json & json_note);
  ~Note() override = default;
  
  [[nodiscard]] auto symbol() const -> std::string override;
};
