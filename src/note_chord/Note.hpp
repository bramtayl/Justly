#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "note_chord/NoteChord.hpp"

struct Note : NoteChord {
  Note() = default;
  explicit Note(const nlohmann::json &json_note);
  ~Note() override = default;
};

[[nodiscard]] auto notes_to_json(const std::vector<Note> &notes,
                                 size_t first_note_number,
                                 size_t number_of_notes) -> nlohmann::json;

void json_to_notes(std::vector<Note> &new_notes,
                   const nlohmann::json &json_notes);