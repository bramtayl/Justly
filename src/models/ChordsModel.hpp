#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <cstddef>                // for size_t
#include <nlohmann/json_fwd.hpp>  // for json

#include "justly/NoteChordField.hpp"
#include "justly/TreeLevel.hpp"  // for TreeLevel
#include "other/CellIndex.hpp"    // for CellIndex

class QObject;
class QUndoStack;
struct Song;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

  Song *song_pointer;
  QUndoStack *undo_stack_pointer;

  [[nodiscard]] auto make_chord_index(int parent_number) const -> QModelIndex;
  [[nodiscard]] auto to_song_index(const QModelIndex &index) const -> CellIndex;

 public:
  explicit ChordsModel(Song *song_pointer_input,
                       QUndoStack *undo_stack_pointer_input,
                       QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto get_parent_number(const QModelIndex &index) const -> int;
  [[nodiscard]] auto get_index(
      int chord_number = -1, int note_number = -1,
      NoteChordField column_number = symbol_column) const -> QModelIndex;

  [[nodiscard]] auto copy(size_t first_child_number, size_t number_of_children,
                          int parent_number) const -> nlohmann::json;
  void load_chords(const nlohmann::json &json_song);

  // overrided methods, generally take QModelIndex and are undoable
  [[nodiscard]] auto rowCount(const QModelIndex &parent_index) const
      -> int override;
  [[nodiscard]] auto columnCount(const QModelIndex &parent) const
      -> int override;

  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto index(int child_number, int note_chord_field,
                           const QModelIndex &parent) const
      -> QModelIndex override;

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;

  auto insertRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  auto removeRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;

  // direct methods: generally take SongIndexes or parent_numbers and are not
  // undoable
  void insert(size_t first_child_number, const nlohmann::json &json_children,
              int parent_number);
  void remove(size_t first_child_number, size_t number_of_children,
              int parent_number);

  void set_cell(const CellIndex &index, const QVariant &new_value);
};

[[nodiscard]] auto get_level(QModelIndex index) -> TreeLevel;

[[nodiscard]] auto verify_children(const QModelIndex &parent_index,
                                   const nlohmann::json &json_children) -> bool;
