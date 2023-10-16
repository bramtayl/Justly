#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for ItemFlags, Orientation
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <gsl/pointers>           // for not_null
#include <nlohmann/json_fwd.hpp>  // for json

#include "notechord/NoteChord.h"  // for TreeLevel
#include "utilities/SongIndex.h"  // for SongIndex

class QObject;
class QUndoStack;
class Song;

class ChordsModel : public QAbstractItemModel {
  Q_OBJECT

 private:
  gsl::not_null<Song *> song_pointer;
  gsl::not_null<QUndoStack *> undo_stack_pointer;
  [[nodiscard]] auto get_tree_index(const SongIndex &index) const
      -> QModelIndex;
  [[nodiscard]] auto get_chord_index(int chord_number) const -> QModelIndex;
  [[nodiscard]] auto get_song_index(const QModelIndex &index) const
      -> SongIndex;
  [[nodiscard]] auto copy_json_children(int first_child_number,
                                        int number_of_children,
                                        int chord_number) const
      -> nlohmann::json;

 public:
  explicit ChordsModel(gsl::not_null<Song *> song_pointer_input,
                       gsl::not_null<QUndoStack *> undo_stack_pointer_input,
                       QObject *parent_pointer_input = nullptr);

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
  [[nodiscard]] static auto get_level(QModelIndex index) -> TreeLevel;

  [[nodiscard]] static auto verify_json_children(
      const QModelIndex &parent_index, const nlohmann::json &json_children)
      -> bool;

  [[nodiscard]] auto get_chord_number(const QModelIndex &index) const -> int;

  void begin_reset_model();
  void end_reset_model();
};
