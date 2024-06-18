#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qcolor.h>              // for QColor
#include <qnamespace.h>          // for ItemFlags, Orientation, black, ligh...
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <nlohmann/json_fwd.hpp>  // for json

#include "src/SongIndex.h"  // for SongIndex
#include "justly/TreeLevel.h"  // for TreeLevel

class QObject;
class QUndoStack;
struct Song;

const auto NON_DEFAULT_COLOR = QColor(Qt::black);
const auto DEFAULT_COLOR = QColor(Qt::lightGray);

const auto NOTE_CHORD_COLUMNS = 7;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

  Song *song_pointer;
  QUndoStack *undo_stack_pointer;

  [[nodiscard]] auto make_chord_index(int parent_number) const -> QModelIndex;

  [[nodiscard]] auto get_song_index(const QModelIndex &index) const
      -> SongIndex;

 public:
  explicit ChordsModel(Song *song_pointer_input,
                       QUndoStack *undo_stack_pointer_input,
                       QObject *parent_pointer_input = nullptr);

  // overrided methods
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
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  auto insertRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  auto removeRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;

  [[nodiscard]] auto copy(int first_child_number, int number_of_children,
                          int parent_number) const -> nlohmann::json;

  void set_cell(const SongIndex &index, const QVariant &new_value);
  void insert(int first_child_number, const nlohmann::json &json_children,
              int parent_number);
  void remove(int first_child_number, int number_of_children, int parent_number);
  void insert_empty(int first_child_number, int number_of_children,
                    int parent_number);

  [[nodiscard]] auto get_parent_number(const QModelIndex &index) const -> int;

  void load_chords(const nlohmann::json &);
};

[[nodiscard]] auto get_level(QModelIndex index) -> TreeLevel;
[[nodiscard]] auto verify_children(const QModelIndex &parent_index,
                                   const nlohmann::json &json_children) -> bool;
