#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariant>
#include <Qt>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "justly/NoteChordColumn.hpp"
#include "note_chord/Chord.hpp"

struct Instrument;
struct Interval;
struct Note;
struct NoteChord;
struct Percussion;
struct Rational;
class QUndoStack;
class QWidget;
struct RowRange;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> size_t;

[[nodiscard]] auto to_note_chord_column(int column) -> NoteChordColumn;

[[nodiscard]] auto is_root_index(const QModelIndex &index) -> bool;
[[nodiscard]] auto valid_is_chord_index(const QModelIndex &index) -> bool;

[[nodiscard]] auto get_midi(double key) -> double;

struct ChordsModel : public QAbstractItemModel {
  Q_OBJECT

public:
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
  void set_chord_cell(size_t chord_number, NoteChordColumn note_chord_column,
                      const QVariant &new_value);
  void set_note_instrument(size_t chord_number, size_t note_number,
                           const Instrument *new_instrument_pointer);
  void set_note_interval(size_t chord_number, size_t note_number,
                         const Interval &new_interval);
  void set_note_percussion(size_t chord_number, size_t note_number,
                           const Percussion *new_percussion_pointer);
  void set_note_beats(size_t chord_number, size_t note_number,
                      const Rational &new_beats);
  void set_note_velocity_ratio(size_t chord_number, size_t note_number,
                               const Rational &new_velocity_ratio);
  void set_note_tempo_ratio(size_t chord_number, size_t note_number,
                            const Rational &new_tempo_ratio);
  void set_note_words(size_t chord_number, size_t note_number,
                      const QString &new_words);
  void change_to_interval(size_t chord_number, size_t note_number,
                          const Instrument *instrument_pointer,
                          const Interval &new_interval);
  void change_to_percussion(size_t chord_number, size_t note_number,
                            const Instrument *instrument_pointer,
                            const Percussion *percussion_pointer);

  void replace_cell_ranges(const std::vector<RowRange> &row_ranges,
                           const std::vector<NoteChord> &note_chords,
                           NoteChordColumn left_column,
                           NoteChordColumn right_column);
  void insert_chord(size_t first_chord_number, const Chord &new_chords);
  void insert_chords(size_t first_chord_number,
                     const std::vector<Chord> &new_chords);
  void append_json_chords(const nlohmann::json &json_chords);
  void remove_chords(size_t first_chord_number, size_t number_of_chords);
  void insert_note(size_t chord_number, size_t note_number,
                   const Note &new_note);
  void insert_notes(size_t chord_number, size_t first_note_number,
                    const std::vector<Note> &new_notes);
  void remove_notes(size_t chord_number, size_t first_note_number,
                    size_t number_of_notes);
};
