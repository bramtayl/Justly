#pragma once

#include <qnamespace.h>
#include <qvariant.h>  // for QVariant

#include <memory>                 // for unique_ptr
#include <nlohmann/json_fwd.hpp>  // for json
#include <vector>                 // for vector

#include "notechord/NoteChord.h"    // for NoteChord, TreeLevel
#include "utilities/StableIndex.h"  // for StableIndex

class TreeNode {
 private:
  TreeNode *parent_pointer = nullptr;
  // pointer so it can be a note or a chord
  std::unique_ptr<NoteChord> note_chord_pointer;
  // pointers so they can be notes or chords
  std::vector<std::unique_ptr<TreeNode>> child_pointers;

 public:
  explicit TreeNode(TreeNode *parent_pointer_input = nullptr);

  [[nodiscard]] auto get_child_pointers() const
      -> const std::vector<std::unique_ptr<TreeNode>> &;
  void remove_children(int first_child_number, int number_of_children);
  void load_from(const nlohmann::json &json_object);
  void insert_empty_children(int first_child_number, int number_of_children);

  [[nodiscard]] auto get_row() const -> int;
  [[nodiscard]] auto number_of_children() const -> int;

  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto get_level() const -> TreeLevel;
  [[nodiscard]] auto is_root() const -> bool;
  [[nodiscard]] auto data(NoteChordField column, Qt::ItemDataRole role) const
      -> QVariant;
  [[nodiscard]] auto get_stable_index(NoteChordField column) const
      -> StableIndex;
  void setData(NoteChordField column, const QVariant &new_value);
  [[nodiscard]] auto copy_json_children(int first_child_number,
                                        int number_of_children) const
      -> nlohmann::json;
  void save_to(nlohmann::json *json_object_pointer) const;
  void insert_json_children(int first_child_number,
                            const nlohmann::json &insertion);
  [[nodiscard]] auto verify_json_children(
      const nlohmann::json &paste_json) const -> bool;

  [[nodiscard]] auto get_const_parent() const -> const TreeNode &;

  auto get_note_chord() -> NoteChord &;

  [[nodiscard]] auto get_const_note_chord() const -> const NoteChord &;
};
