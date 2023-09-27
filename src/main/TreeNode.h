#pragma once

#include <qvariant.h>  // for QVariant

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "notechord/NoteChord.h"    // for NoteChord, TreeLevel
#include "utilities/StableIndex.h"  // for StableIndex

class TreeNode {
 public:
  TreeNode * parent_pointer = nullptr;
  // pointer so it can be a note or a chord
  std::unique_ptr<NoteChord> note_chord_pointer;
  // pointers so they can be notes or chords
  std::vector<std::unique_ptr<TreeNode>> child_pointers;

  explicit TreeNode(TreeNode *parent_pointer_input = nullptr);
  void remove_children(int first_index, int number_of_children);
  void load_from(const nlohmann::json &json_object);
  void remove_save_children(
      int first_index, int number_of_children,
      std::vector<std::unique_ptr<TreeNode>> &deleted_children);
  void insert_empty_children(int first_index, int number_of_children);

  [[nodiscard]] auto get_row() const -> int;
  [[nodiscard]] auto verify_child_at(int index) const -> bool;
  [[nodiscard]] auto verify_insertable_at(int index) const -> bool;
  void insert_children(int first_index,
                       std::vector<std::unique_ptr<TreeNode>> &insertion);
  [[nodiscard]] auto number_of_children() const -> int;

  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto get_level() const -> TreeLevel;
  [[nodiscard]] auto is_root() const -> bool;
  [[nodiscard]] auto verify_not_root() const -> bool;
  [[nodiscard]] auto data(int column, int role) const -> QVariant;
  [[nodiscard]] auto get_stable_index(int column) const -> StableIndex;
  void setData(int column, const QVariant &new_value) const;
  [[nodiscard]] auto copy_json_children(int first_index, int number_of_children)
      -> nlohmann::json;
  void save_to(nlohmann::json &json_object) const;
  void insert_json_children(int first_index, const nlohmann::json &insertion);
  [[nodiscard]] auto verify_json_children(
      const nlohmann::json &paste_json) const -> bool;
};
