#pragma once

#include <nlohmann/json.hpp>

#include "note_chord/NoteChord.hpp"

struct Note : NoteChord {
  Note() = default;
  explicit Note(const nlohmann::json &json_note);
  ~Note() override = default;
};
