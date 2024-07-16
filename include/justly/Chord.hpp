#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Chord : NoteChord {
  std::vector<Note> notes;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);
  ~Chord() override = default;

  [[nodiscard]] auto symbol() const -> std::string override;
  [[nodiscard]] auto json() const -> nlohmann::json override;
};

auto get_chord_schema() -> const nlohmann::json &;
