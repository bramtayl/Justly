#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"

struct Chord : NoteChord {
public:
  std::vector<Note> notes;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);
  ~Chord() override = default;

  [[nodiscard]] auto is_chord() const -> bool override;
};

[[nodiscard]] auto chords_to_json(const std::vector<Chord> &chords,
                                  size_t first_chord_number,
                                  size_t number_of_chords) -> nlohmann::json;

void json_to_chords(std::vector<Chord> &new_chords,
                    const nlohmann::json &json_chords);