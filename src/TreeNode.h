#pragma once

#include <qvariant.h>  // for QVariant
#include <qjsonarray.h>
#include <qjsonobject.h>

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"    // for NoteChord, TreeLevel
#include "StableIndex.h"  // for StableIndex

class Instrument;

class TreeNode {
 public:

  TreeNode *const parent_pointer = nullptr;
  // pointer so it can be a note or a chord
  const std::unique_ptr<NoteChord> note_chord_pointer;
  // pointers so they can be notes or chords
  std::vector<std::unique_ptr<TreeNode>> child_pointers;

  explicit TreeNode(TreeNode *parent_pointer_input = nullptr);
  void remove_children(int first_index, int number_of_children);
  void load_from(const QJsonObject& json_object);
  void remove_save_children(
      int first_index, int number_of_children,
      std::vector<std::unique_ptr<TreeNode>> &deleted_children);
  void insert_empty_children(int first_index, int number_of_children);

  [[nodiscard]] auto is_at_row() const -> int;
  [[nodiscard]] auto verify_child_at(int first_index) const -> bool;
  [[nodiscard]] auto verify_insertable_at(int first_index) const -> bool;
  void insert_children(int first_index,
                       std::vector<std::unique_ptr<TreeNode>> &insertion);
  [[nodiscard]] auto get_child_count() const -> int;

  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto get_level() const -> TreeLevel;
  [[nodiscard]] auto is_root() const -> bool;
  [[nodiscard]] auto verify_not_root() const -> bool;
  [[nodiscard]] auto data(int column, int role) const -> QVariant;
  [[nodiscard]] auto get_stable_index(int column) const -> StableIndex;
  void setData(int column, const QVariant &new_value);
  [[nodiscard]] auto copy_json_children(int first_index, int number_of_children) -> QJsonArray;
  void save_to(QJsonObject& json_object) const;
  void insert_json_children(int first_index, const QJsonArray& insertion);
  [[nodiscard]] auto verify_json_children(const QJsonArray& insertion, const std::vector<Instrument> &instruments) const -> bool;
};

auto new_child_pointer(TreeNode *parent_pointer) -> std::unique_ptr<NoteChord>;


