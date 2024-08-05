#pragma once

#include <QString>
#include <QVariant>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Note.hpp"
#include "note_chord/NoteChord.hpp"

struct Chord : NoteChord {
private:
  std::vector<Note> notes;
  void check_note_number(size_t note_number) const;
  void check_note_number_end(size_t note_number) const;
  void check_note_range(size_t first_note_number, size_t number_of_notes) const;

  [[nodiscard]] auto get_note(size_t note_number) -> Note &;

public:
  Chord() = default;
  explicit Chord(const nlohmann::json &json_chord);
  ~Chord() override = default;

  [[nodiscard]] auto is_chord() const -> bool override;
  [[nodiscard]] auto get_symbol() const -> QString override;
  [[nodiscard]] auto to_json() const -> nlohmann::json override;

  [[nodiscard]] auto get_number_of_notes() const -> size_t;

  [[nodiscard]] auto get_const_note(size_t note_number) const -> const Note &;

  void set_note_data(size_t note_number, NoteChordColumn note_chord_column,
                     const QVariant &new_value);

  void
  copy_notes_to_notechords(size_t first_note_number, size_t number_of_notes,
                           std::vector<NoteChord> *note_chords_pointer) const;

  [[nodiscard]] auto
  copy_notes(size_t first_note_number,
             size_t number_of_notes) const -> std::vector<Note>;
  [[nodiscard]] auto
  copy_notes_to_json(size_t first_note_number,
                     size_t number_of_notes) const -> nlohmann::json;

  void insert_notes(size_t first_note_number,
                    const std::vector<Note> &new_notes);
  void remove_notes(size_t first_note_number, size_t number_of_notes);

  void replace_note_cells(size_t first_note_number, size_t number_of_children,
                          NoteChordColumn left_field,
                          NoteChordColumn right_field,
                          const std::vector<NoteChord> &note_chords,
                          size_t first_note_chord_number);
};