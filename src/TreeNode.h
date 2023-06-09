#pragma once

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "NoteChord.h"  // for NoteChord
class QJsonObject;      // lines 9-9
class QString;          // lines 10-10

const auto ROOT_LEVEL = 0;

class TreeNode {
 public:
  // pointer so it can be null for root
  const std::vector<std::unique_ptr<const QString>>& instrument_pointers;
  const QString& default_instrument;
  
  TreeNode *const parent_pointer = nullptr;
  // pointer so it can be a note or a chord
  const std::unique_ptr<NoteChord> note_chord_pointer;
  // pointers so they can be notes or chords
  std::vector<std::unique_ptr<TreeNode>> child_pointers;

  explicit TreeNode(const std::vector<std::unique_ptr<const QString>>& instrument_pointers,
                        const QString& default_instrument,
                    TreeNode *parent_pointer_input = nullptr);
  TreeNode(const TreeNode &copied, TreeNode *parent_pointer_input);
  void copy_children(const TreeNode &copied);

  auto new_child_pointer(TreeNode *parent_pointer)
      -> std::unique_ptr<NoteChord>;
  [[nodiscard]] auto copy_note_chord_pointer() const
      -> std::unique_ptr<NoteChord>;
  [[nodiscard]] auto is_at_row() const -> int;
  auto assert_child_at(size_t position) const -> void;
  auto assert_insertable_at(size_t position) const -> void;
  [[nodiscard]] auto get_child_count() const -> size_t;
  auto load_children(const QJsonObject &json_object) -> void;
  auto save_children(QJsonObject &json_object) const -> void;
  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto get_level() const -> int;
};
