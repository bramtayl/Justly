#pragma once

#include <QString>
#include <QVariant>

#include "items_model/ItemsModel.hpp"
#include "justly/ChordColumn.hpp"

struct Chord;
class QObject;
class QModelIndex;
class QUndoStack;

template <typename T> class QList;

[[nodiscard]] auto to_chord_column(int column) -> ChordColumn;

[[nodiscard]] auto get_midi(double key) -> double;

struct ChordsModel : public ItemsModel<Chord> {
  QUndoStack *const undo_stack_pointer;

  double gain;
  double starting_key;
  double starting_velocity;
  double starting_tempo;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QList<Chord>* chords_pointer_input,
                       QObject *parent_pointer = nullptr);
  
  [[nodiscard]] auto get_instrument_column() const -> int override;
  [[nodiscard]] auto get_percussion_set_column() const -> int override;
  [[nodiscard]] auto get_percussion_instrument_column() const -> int override;
  [[nodiscard]] auto get_interval_column() const -> int override;
  [[nodiscard]] auto get_beats_column() const -> int override;
  [[nodiscard]] auto get_tempo_ratio_column() const -> int override;
  [[nodiscard]] auto get_velocity_ratio_column() const -> int override;
  [[nodiscard]] auto get_words_column() const -> int override;

  // override functions
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto is_column_editable(int column_number) const -> bool override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
  void set_cells(int first_item_number, int left_column,
                             int right_column,
                             const QList<Chord> &new_items) override;
};

[[nodiscard]] auto get_key_text(const ChordsModel &chords_model,
                                int last_chord_number,
                                double ratio = 1) -> QString;

