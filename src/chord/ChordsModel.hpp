#pragma once

#include <QString>

#include "chord/Chord.hpp"
#include "items_model/ItemsModel.hpp"

class QObject;
class QModelIndex;
class QUndoStack;

template <typename T> class QList;

[[nodiscard]] auto get_midi(double key) -> double;

struct ChordsModel : public ItemsModel<Chord> {
  double gain;
  double starting_key;
  double starting_velocity;
  double starting_tempo;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QList<Chord> *chords_pointer_input,
                       QObject *parent_pointer = nullptr);

  // override functions
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto
  get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto
  is_column_editable(int column_number) const -> bool override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};

[[nodiscard]] auto get_key_text(const ChordsModel &chords_model,
                                int last_chord_number,
                                double ratio = 1) -> QString;
