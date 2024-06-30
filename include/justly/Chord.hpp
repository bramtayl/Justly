#pragma once

#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>                 // for vector

#include "justly/Note.hpp"       // for Note
#include "justly/NoteChord.hpp"  // for NoteChord
#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Chord : NoteChord {
  std::vector<Note> notes;

  Chord() = default;
  explicit Chord(const nlohmann::json& json_chord);
  ~Chord() override = default;

  [[nodiscard]] auto symbol() const -> std::string override;
  [[nodiscard]] auto json() const -> nlohmann::json override;
};
