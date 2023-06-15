#include "TreeNode.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <qjsonarray.h>      // for QJsonArray, QJsonArray::iterator
#include <qjsonobject.h>     // for QJsonObject
#include <qjsonvalue.h>      // for QJsonValueRef, QJsonValue
#include <qstring.h>         // for QString

#include <cmath>               // for pow
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <memory>              // for unique_ptr, make_unique, operator==
#include <utility>             // for move

#include "Chord.h"      // for Chord
#include "Note.h"       // for Note
#include "NoteChord.h"  // for NoteChord, root_level, OCTAVE_RATIO
#include "Utilities.h"  // for error_root, error_row

auto new_child_pointer(TreeNode *parent_pointer, const QString& default_instrument)
    -> std::unique_ptr<NoteChord> {
  // if parent is null, this is the root
  // the root will have no data
  if (parent_pointer == nullptr) {
    return nullptr;
  }
  auto parent_level = parent_pointer -> get_level();
  if (parent_level == root_level) {
    return std::make_unique<Chord>(default_instrument);
  }
  if (parent_level == chord_level) {
    return std::make_unique<Note>(default_instrument);
  }
  qCritical("Notes can't have children!");
  return nullptr;
}

TreeNode::TreeNode(
    const std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    const QString &default_instrument, TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      instrument_pointers(instrument_pointers),
      default_instrument(default_instrument),
      note_chord_pointer(new_child_pointer(parent_pointer_input, default_instrument)){};

auto TreeNode::copy_note_chord_pointer() const -> std::unique_ptr<NoteChord> {
  if (get_level() == root_level) {
    error_level(root_level);
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
      default_instrument(copied.default_instrument),
      note_chord_pointer(copied.copy_note_chord_pointer()) {
  copy_children(copied);
}

auto TreeNode::is_at_row() const -> int {
  // parent_pointer is null for the root item
  // the root item is always at row 0
  if (get_level() == root_level) {
    error_level(root_level);
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
  if (get_level() == root_level) {
    error_level(root_level);
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
