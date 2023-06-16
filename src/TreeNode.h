#pragma once

#include <cstddef>      // for size_t
#include <memory>       // for unique_ptr
#include <vector>       // for vector

#include "NoteChord.h"  // for NoteChord
#include "Utilities.h"  // for TreeLevel

class QJsonObject;      // lines 9-9
class QString;          // lines 10-10



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

  [[nodiscard]] auto copy_note_chord_pointer() const
      -> std::unique_ptr<NoteChord>;
  [[nodiscard]] auto is_at_row() const -> int;
  auto verify_child_at(size_t position) const -> bool;
  auto verify_insertable_at(size_t position) const -> bool;
  [[nodiscard]] auto get_child_count() const -> size_t;

  [[nodiscard]] auto get_ratio() const -> double;
  [[nodiscard]] auto get_level() const -> TreeLevel;
  [[nodiscard]] auto is_root() const -> bool;
  [[nodiscard]] auto verify_not_root() const -> bool;
};

auto new_child_pointer(TreeNode *parent_pointer, const QString& default_instrument)
    -> std::unique_ptr<NoteChord>;