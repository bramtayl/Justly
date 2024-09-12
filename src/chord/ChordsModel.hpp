#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <QList>

#include "chord/Chord.hpp"
#include "justly/ChordColumn.hpp"

class QUndoStack;
class QWidget;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> qsizetype;

[[nodiscard]] auto to_chord_column(int column) -> ChordColumn;

[[nodiscard]] auto get_midi(double key) -> double;

struct ChordsModel : public QAbstractTableModel {
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;
  QList<Chord> chords;

  double gain;
  double starting_key;
  double starting_velocity;
  double starting_tempo;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);

  // override functions
  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto headerData(int column, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex & /*index*/) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;

  // internal functions
  void edited_chords_cells(qsizetype first_chord_number, qsizetype number_of_chords,
                           ChordColumn left_column, ChordColumn right_column);

  void begin_insert_rows(qsizetype first_chord_number, qsizetype number_of_chords);
  void end_insert_rows();

  void begin_remove_rows(qsizetype first_chord_number, qsizetype number_of_chords);
  void end_remove_rows();

  void begin_reset_model();
  void end_reset_model();
};

void insert_chords(ChordsModel *chords_model_pointer, qsizetype first_chord_number,
                   const QList<Chord> &new_chords);
void remove_chords(ChordsModel *chords_model_pointer, qsizetype first_chord_number,
                   qsizetype number_of_chords);
