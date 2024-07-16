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

  [[nodiscard]] auto
  notes_to_json(size_t first_child_number,
                size_t number_of_children) const -> nlohmann::json;
  void notes_from_json(size_t first_child_number,
                       const nlohmann::json &json_notes);
};

auto get_chord_schema() -> const nlohmann::json &;
