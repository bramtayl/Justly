#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT, signals
#include <qvariant.h>            // for QVariant

#include <cstddef>                // for size_t
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>                 // for vector

#include "justly/CellIndex.hpp"         // for CellIndex
#include "justly/Chord.hpp"             // for Chord
#include "justly/CopyType.hpp"          // for CopyType, no_copy
#include "justly/NoteChordField.hpp"    // for symbol_column, NoteChordField
#include "justly/TreeLevel.hpp"         // for TreeLevel
#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT

class QWidget;
class QUndoStack;
struct NoteChord;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

 private:
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;

  [[nodiscard]] auto make_chord_index(int parent_number) const -> QModelIndex;
  [[nodiscard]] auto to_cell_index(const QModelIndex &index) const -> CellIndex;
  [[nodiscard]] auto get_const_note_chord_pointer(int parent_number,
                                                  size_t child_number) const
      -> const NoteChord *;

  [[nodiscard]] auto copy_rows_to(size_t first_child_number,
                                  size_t number_of_children, int parent_number)
      -> nlohmann::json;

 public:
  std::vector<Chord> chords;
  CopyType copy_type = no_copy;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);
  [[nodiscard]] auto get_index(
      int parent_number, size_t child_number,
      NoteChordField note_chord_field = symbol_column) const -> QModelIndex;

  void copy_rows(size_t first_child_number, size_t number_of_children,
                 int parent_number);
  void load_chords(const nlohmann::json &json_song);

  // overrided methods, generally take QModelIndex and are undoable
  [[nodiscard]] auto rowCount(const QModelIndex &parent_index) const
      -> int override;
  [[nodiscard]] auto columnCount(const QModelIndex &parent) const
      -> int override;

  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto index(int child_number, int column,
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

  // direct methods: generally take CellIndex or parent_number and are not
  // undoable
  void insert_remove_directly(size_t first_child_number,
                              const nlohmann::json &json_children,
                              int parent_number, bool should_insert);
  void set_cell_directly(const CellIndex &cell_index,
                         const QVariant &new_value);

  void paste_rows(int first_child_number, const QModelIndex &parent_index);

  void paste_cell(const QModelIndex &index);
  void paste_rows_text(size_t first_child_number, const std::string &text,
                       int parent_number);
  void copy_cell(QModelIndex index);

  [[nodiscard]] auto get_number_of_children(int parent_number) const -> size_t;

 signals:
  void copy_type_changed(CopyType new_copy_type);
};

[[nodiscard]] auto JUSTLY_EXPORT get_level(QModelIndex index) -> TreeLevel;

auto JUSTLY_EXPORT to_parent_number(const QModelIndex &index) -> int;
