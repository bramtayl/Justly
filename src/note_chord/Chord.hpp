#pragma once

#include <QString>
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
  [[nodiscard]] auto get_symbol() const -> QString override;
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
};
