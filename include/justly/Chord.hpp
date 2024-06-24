#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>                 // for vector

#include "justly/Note.hpp"       // for Note
#include "justly/NoteChord.hpp"  // for NoteChord
#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Chord : NoteChord {
  std::vector<std::unique_ptr<Note>> note_pointers;

  Chord() = default;
  explicit Chord(const nlohmann::json& json_chord);
  ~Chord() override = default;
  NO_MOVE_COPY(Chord)

  [[nodiscard]] auto symbol() const -> std::string override;
  [[nodiscard]] auto json() const -> nlohmann::json override;
};
