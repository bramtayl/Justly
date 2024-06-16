#pragma once

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>                 // for vector

#include "justly/Note.h"       // for Note
#include "justly/NoteChord.h"  // for NoteChord

struct Chord : NoteChord {
  std::vector<std::unique_ptr<Note>> note_pointers;

  Chord() = default;
  explicit Chord(const nlohmann::json &);
  ~Chord() override = default;

  [[nodiscard]] auto symbol() const -> std::string override;
  [[nodiscard]] auto json() const -> nlohmann::json override;
};
