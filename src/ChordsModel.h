#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qcolor.h>              // for QColor
#include <qnamespace.h>          // for ItemFlags, Orientation, black
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <algorithm>              // for transform, find_if
#include <cstddef>                // for size_t
#include <memory>                 // for unique_ptr, allocator_traits<>::v...
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "justly/Chord.h"           // for Chord, children_to_json
#include "justly/Note.h"            // for Note
#include "justly/NoteChordField.h"  // for symbol_column
#include "justly/Song.h"            // for Song
#include "justly/TreeLevel.h"       // for chord_level, note_level, root_level
#include "justly/macros.h"          // for NO_MOVE_COPY
#include "src/SongIndex.h"          // for SongIndex

class QObject;
class QUndoStack;

const auto NON_DEFAULT_COLOR = QColor(Qt::black);
const auto DEFAULT_COLOR = QColor(Qt::lightGray);

const auto NOTE_CHORD_COLUMNS = 7;

const auto LARGE_FONT_SIZE = 18;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

  Song *song_pointer;
  QUndoStack *undo_stack_pointer;

  [[nodiscard]] inline auto get_tree_index(const SongIndex &song_index) const
      -> QModelIndex {
    auto chord_number = song_index.chord_number;
    if (chord_number == -1) {
      // it's root, so return an invalid index
      return {};
    }
    auto note_chord_field = song_index.note_chord_field;
    auto note_number = song_index.note_number;
    return note_number == -1
               // for chords, the row is the chord number, and the parent
               // pointer is null
               ? createIndex(chord_number, note_chord_field, nullptr)
               // for notes, the row is the note number, and the parent pointer
               // is the chord pointer
               : createIndex(
                     note_number, note_chord_field,
                     song_pointer
                         ->chord_pointers[static_cast<size_t>(chord_number)]
                         .get());
  };

  [[nodiscard]] inline auto get_chord_index(int chord_number) const
      -> QModelIndex {
    // for root, use an empty index
    return chord_number == -1
               ? QModelIndex()
               // for chords, the parent pointer is null
               : createIndex(chord_number, symbol_column, nullptr);
  }

  [[nodiscard]] auto get_song_index(const QModelIndex &index) const
      -> SongIndex;

  [[nodiscard]] inline auto copy_json_children(int first_child_number,
                                               int number_of_children,
                                               int chord_number) const
      -> nlohmann::json {
    return chord_number == -1
               // for root
               ? children_to_json(song_pointer->chord_pointers,
                                  first_child_number, number_of_children)
               // for a chord
               : children_to_json(
                     song_pointer
                         ->chord_pointers[static_cast<size_t>(chord_number)]
                         ->note_pointers,
                     first_child_number, number_of_children);
  }

  static inline auto get_text_color(bool is_default) -> QColor {
    return is_default ? DEFAULT_COLOR : NON_DEFAULT_COLOR;
  }

 public:
  explicit ChordsModel(Song *song_pointer_input,
                       QUndoStack *undo_stack_pointer_input,
                       QObject *parent_pointer_input = nullptr);
  NO_MOVE_COPY(ChordsModel);

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
  void insertJsonChildren(int first_child_number,
                          const nlohmann::json &json_children,
                          const QModelIndex &parent_index);
  [[nodiscard]] auto copyJsonChildren(int first_child_number,
                                      int number_of_children,
                                      const QModelIndex &parent_index) const
      -> nlohmann::json;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;

  void set_data_directly(const SongIndex &index, const QVariant &new_value);
  void insert_json_children_directly(int first_child_number,
                                     const nlohmann::json &json_children,
                                     int chord_number);
  void remove_children_directly(int first_child_number, int number_of_children,
                                int chord_number);
  void insert_empty_children_directly(int first_child_number,
                                      int number_of_children, int chord_number);

  [[nodiscard]] static inline auto get_level(QModelIndex index) -> TreeLevel {
    // root will be an invalid index
    return index.row() == -1 ? root_level
           // chords have null parent pointers
           : index.internalPointer() == nullptr ? chord_level
                                                : note_level;
  }

  [[nodiscard]] static auto verify_json_children(
      const QModelIndex &parent_index, const nlohmann::json &json_children)
      -> bool;

  [[nodiscard]] inline auto get_chord_number(const QModelIndex &index) const
      -> int {
    auto *chord_pointer = index.internalPointer();
    auto &chord_pointers = song_pointer->chord_pointers;
    switch (ChordsModel::get_level(index)) {
      case root_level:
        return -1;
      case chord_level:
        return index.row();
      case note_level:
        return static_cast<int>(
            std::find_if(
                chord_pointers.begin(), chord_pointers.end(),
                [chord_pointer](std::unique_ptr<Chord> &maybe_chord_pointer) {
                  return maybe_chord_pointer.get() == chord_pointer;
                }) -
            chord_pointers.begin());
      default:
        return {};
    }
  }

  void begin_reset_model();
  void end_reset_model();
};
