#pragma once

#include <cstddef>  // for int
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "NoteChord.h"  // for NoteChord
#include "StableIndex.h"

class Instrument;

class TreeNode {
 public:
  // pointer so it can be null for root
  const std::vector<Instrument> &instruments;

  TreeNode *const parent_pointer = nullptr;
  // pointer so it can be a note or a chord
  const std::unique_ptr<NoteChord> note_chord_pointer;
  // pointers so they can be notes or chords
  std::vector<std::unique_ptr<TreeNode>> child_pointers;

  explicit TreeNode(const std::vector<Instrument> &instruments_input,
                    TreeNode *parent_pointer_input = nullptr);
  TreeNode(const TreeNode &copied, TreeNode *parent_pointer_input);
  void copy_children(const TreeNode &copied);
  void remove_children(int first_index, int number_of_children);
  void remove_save_children(int first_index, int number_of_children, std::vector<std::unique_ptr<TreeNode>> &deleted_rows);
  void insert_empty_children(int first_index, int number_of_children);

  [[nodiscard]] auto copy_note_chord_pointer() const
      -> std::unique_ptr<NoteChord>;
  [[nodiscard]] auto is_at_row() const -> int;
  [[nodiscard]] auto verify_child_at(int position) const -> bool;
  [[nodiscard]] auto verify_insertable_at(int position) const -> bool;
  void insert_children(int first_index, std::vector<std::unique_ptr<TreeNode>> &insertion);
  [[nodiscard]] auto get_child_count() const -> int;

  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto get_level() const -> TreeLevel;
  [[nodiscard]] auto is_root() const -> bool;
  [[nodiscard]] auto verify_not_root() const -> bool;
  [[nodiscard]] auto data(int column, int role) const -> QVariant;
  [[nodiscard]] auto get_stable_index(int column) const -> StableIndex;
  void setData(int column, const QVariant &new_value);

};

auto new_child_pointer(TreeNode *parent_pointer) -> std::unique_ptr<NoteChord>;
