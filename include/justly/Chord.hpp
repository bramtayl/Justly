#pragma once

#include <QString>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"

struct JUSTLY_EXPORT Chord : NoteChord {
  std::vector<Note> notes;

  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);
  ~Chord() override = default;

  [[nodiscard]] auto symbol() const -> QString override;
  [[nodiscard]] auto json() const -> nlohmann::json override;

  [[nodiscard]] auto get_number_of_notes() const -> size_t;

  void check_note_number(size_t note_number) const;
  void check_note_number_end(size_t note_number) const;
  void check_note_range(size_t first_note_number, size_t number_of_notes) const;

  [[nodiscard]] auto get_const_note(size_t note_number) const -> const Note &;
  [[nodiscard]] auto get_note(size_t note_number) -> Note &;

  void copy_notes_to(size_t first_note_number, size_t number_of_notes,
                     std::vector<NoteChord> *note_chords_pointer) const;

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

  void replace_note_cells(size_t first_note_number, NoteChordField left_field,
                          NoteChordField right_field,
                          const std::vector<NoteChord> &note_chords,
                          size_t first_note_chord_number, size_t write_number);
};

auto get_chords_schema() -> const nlohmann::json &;
