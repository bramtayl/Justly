#include "TreeNode.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <qstring.h>         // for QString

#include <cmath>   // for pow
#include <memory>  // for unique_ptr, make_unique, operator==, all...

#include "Chord.h"      // for Chord
#include "NoteChord.h"  // for NoteChord, OCTAVE_RATIO
#include "Utilities.h"  // for error_row, error_level, root_level, Tree...

auto new_child_pointer(TreeNode *parent_pointer)
    -> std::unique_ptr<NoteChord> {
  // if parent is null, this is the root
  // the root will have no data
  if (parent_pointer == nullptr) {
    return nullptr;
  }
  if (parent_pointer->is_root()) {
    return std::make_unique<Chord>();
  }
  return parent_pointer->note_chord_pointer->new_child_pointer();
}

TreeNode::TreeNode(
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      instrument_pointers(instrument_pointers),
      note_chord_pointer(
          new_child_pointer(parent_pointer_input)){};

auto TreeNode::copy_note_chord_pointer() const -> std::unique_ptr<NoteChord> {
  if (!(verify_not_root())) {
    return {};
  }
  return note_chord_pointer->copy_pointer();
}

void TreeNode::copy_children(const TreeNode &copied) {
  for (const auto &child_pointer : copied.child_pointers) {
    child_pointers.push_back(std::make_unique<TreeNode>(*child_pointer, this));
  }
}

TreeNode::TreeNode(const TreeNode &copied, TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      instrument_pointers(copied.instrument_pointers),
      note_chord_pointer(copied.copy_note_chord_pointer()) {
  copy_children(copied);
}

auto TreeNode::is_at_row() const -> int {
  // parent_pointer is null for the root item
  // the root item is always at row 0
  if (!(verify_not_root())) {
    return -1;
  }
  auto &siblings = parent_pointer->child_pointers;
  for (auto index = 0; index < siblings.size(); index = index + 1) {
    if (this == siblings[index].get()) {
      return index;
    }
  }
  // TODO: test
  qCritical("Not a child!");
  return -1;
}

auto TreeNode::get_child_count() const -> size_t {
  return child_pointers.size();
};

auto TreeNode::verify_child_at(size_t position) const -> bool {
  if (position < 0 || position >= get_child_count()) {
    error_row(position);
    return false;
  }
  return true;
}

// appending is inserting at the size
auto TreeNode::verify_insertable_at(size_t position) const -> bool {
  if (position < 0 || position > get_child_count()) {
    error_row(position);
    return false;
  }
  return true;
}

auto TreeNode::get_ratio() const -> double {
  if (!(verify_not_root())) {
    return -1;
  }
  return (1.0 * note_chord_pointer->numerator) /
         note_chord_pointer->denominator *
         pow(OCTAVE_RATIO, note_chord_pointer->octave);
}

auto TreeNode::get_level() const -> TreeLevel {
  if (note_chord_pointer == nullptr) {
    return root_level;
  }
  return note_chord_pointer->get_level();
}

auto TreeNode::is_root() const -> bool { return note_chord_pointer == nullptr; }

auto TreeNode::verify_not_root() const -> bool {
  if (is_root()) {
    error_level(root_level);
    return false;
  }
  return true;
}
