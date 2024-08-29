#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <cstddef>
#include <vector>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"

struct Instrument;
struct Interval;
struct Note;
struct Percussion;
class QUndoStack;
class QWidget;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> size_t;

[[nodiscard]] auto to_note_chord_column(int column) -> NoteChordColumn;

[[nodiscard]] auto is_root_index(const QModelIndex &index) -> bool;
[[nodiscard]] auto valid_is_chord_index(const QModelIndex &index) -> bool;

[[nodiscard]] auto get_midi(double key) -> double;

struct ChordsModel : public QAbstractItemModel {
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;
  std::vector<Chord> chords;

  double gain;
  double starting_key;
  double starting_velocity;
  double starting_tempo;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);

  // index functions
  [[nodiscard]] auto get_chord_index(
      size_t chord_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;
  [[nodiscard]] auto get_note_index(
      size_t chord_number, size_t note_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;

  // override functions
  [[nodiscard]] auto
  rowCount(const QModelIndex &parent_index) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;
  [[nodiscard]] auto
  parent(const QModelIndex &index) const -> QModelIndex override;
  [[nodiscard]] auto
  index(int signed_child_number, int column,
        const QModelIndex &parent_index) const -> QModelIndex override;

  [[nodiscard]] auto headerData(int column, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;

  // internal functions
  void edited_chords_cells(size_t first_chord_number, size_t number_of_chords,
                           NoteChordColumn left_column,
                           NoteChordColumn right_column);
  void edited_notes_cells(size_t chord_number, size_t first_note_number,
                          size_t number_of_notes, NoteChordColumn left_column,
                          NoteChordColumn right_column);

  void begin_insert_chords(size_t first_chord_number, size_t number_of_chords);
  void begin_insert_notes(size_t chord_number, size_t first_note_number,
                          size_t number_of_notes);
  void end_insert_rows();

  void begin_remove_chords(size_t first_chord_number, size_t number_of_chords);
  void begin_remove_notes(size_t chord_number, size_t first_note_number,
                          size_t number_of_notes);
  void end_remove_rows();
};

void change_to_interval(ChordsModel *chords_model_pointer, size_t chord_number,
                        size_t note_number,
                        const Instrument *instrument_pointer,
                        const Interval &new_interval);

void change_to_percussion(ChordsModel *chords_model_pointer,
                          size_t chord_number, size_t note_number,
                          const Instrument *instrument_pointer,
                          const Percussion *percussion_pointer);

void insert_chords(ChordsModel *chords_model_pointer, size_t first_chord_number,
                   const std::vector<Chord> &new_chords);
void remove_chords(ChordsModel *chords_model_pointer, size_t first_chord_number,
                   size_t number_of_chords);

void insert_notes(ChordsModel *chords_model_pointer, size_t chord_number,
                  size_t first_note_number, const std::vector<Note> &new_notes);
void remove_notes(ChordsModel *chords_model_pointer, size_t chord_number,
                  size_t first_note_number, size_t number_of_notes);