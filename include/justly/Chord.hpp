#pragma once

#include <QString>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"

struct JUSTLY_EXPORT Chord : NoteChord {
  std::vector<Note> notes;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);
  ~Chord() override = default;

  [[nodiscard]] auto symbol() const -> QString override;
  [[nodiscard]] auto json() const -> nlohmann::json override;

  void check_note_number(size_t note_number) const;
  void check_new_note_number(size_t note_number) const;

  [[nodiscard]] auto get_const_note(size_t note_number) const -> const Note &;
  [[nodiscard]] auto get_note(size_t note_number) -> Note &;

  [[nodiscard]] auto
  copy_notes(size_t first_note_number,
             size_t number_of_notes) const -> std::vector<Note>;
  [[nodiscard]] auto
  copy_notes_to_json(size_t first_note_number,
                     size_t number_of_notes) const -> nlohmann::json;

  void insert_notes(size_t first_note_number,
                    const std::vector<Note> &new_notes);
  void insert_json_notes(size_t first_note_number,
                         const nlohmann::json &json_notes);
  void remove_notes(size_t first_note_number, size_t number_of_notes);
};

auto get_chords_schema() -> const nlohmann::json &;
