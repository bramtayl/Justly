#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <Qt>
#include <QtGlobal>

#include "chord/Chord.hpp"
#include "justly/ChordColumn.hpp"
#include "other/ItemModel.hpp"

class QObject;
class QModelIndex;
class QUndoStack;

[[nodiscard]] auto to_chord_column(int column) -> ChordColumn;

[[nodiscard]] auto get_midi(double key) -> double;

struct ChordsModel : public ItemModel {
  QUndoStack *const undo_stack_pointer;
  QList<Chord> chords;

  double gain;
  double starting_key;
  double starting_velocity;
  double starting_tempo;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QObject *parent_pointer = nullptr);

  // override functions
  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto headerData(int column, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
};

[[nodiscard]] auto get_key_text(const ChordsModel &chords_model,
                                qsizetype last_chord_number,
                                double ratio = 1) -> QString;

void insert_chords(ChordsModel &chords_model, qsizetype first_chord_number,
                   const QList<Chord> &new_chords);
void remove_chords(ChordsModel &chords_model, qsizetype first_chord_number,
                   qsizetype number_of_chords);
