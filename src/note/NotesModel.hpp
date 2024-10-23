#pragma once

#include <QString>
#include <QVariant>

#include "justly/NoteColumn.hpp"
#include "items_model/ItemsModel.hpp"

class QObject;
struct ChordsModel;
struct Note;
template <typename T> class QList;
class QModelIndex;

[[nodiscard]] auto to_note_column(int column) -> NoteColumn;

struct NotesModel : public ItemsModel<Note> {
  ChordsModel *const parent_chords_model_pointer;
  int parent_chord_number = -1;

  explicit NotesModel(ChordsModel *parent_chords_model_pointer_input,
                      QObject *parent_pointer = nullptr);

  [[nodiscard]] auto get_instrument_column() const -> int override;
  [[nodiscard]] auto get_interval_column() const -> int override;
  [[nodiscard]] auto get_beats_column() const -> int override;
  [[nodiscard]] auto get_tempo_ratio_column() const -> int override;
  [[nodiscard]] auto get_velocity_ratio_column() const -> int override;
  [[nodiscard]] auto get_words_column() const -> int override;

  // override functions
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto get_column_name(int column_number) const -> QString override;
  
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
};
