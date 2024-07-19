#pragma once

#include <QString>
#include <nlohmann/json.hpp>

#include "justly/NoteChord.hpp"
#include "justly/JUSTLY_EXPORT.hpp"

struct JUSTLY_EXPORT Note : NoteChord {
  Note() = default;
  explicit Note(const nlohmann::json &json_note);
  ~Note() override = default;

  [[nodiscard]] auto symbol() const -> QString override;
};

[[nodiscard]] auto get_notes_schema() -> const nlohmann::json &;
