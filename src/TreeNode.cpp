#include "TreeNode.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <qjsonarray.h>      // for QJsonArray
#include <qjsonobject.h>     // for QJsonObject
#include <qjsonvalue.h>      // for QJsonValue, QJsonValueRef

#include <cmath>               // for pow
#include <ext/alloc_traits.h>  // for __alloc_traits<>::value_type
#include <memory>              // for unique_ptr, make_unique, operator==
#include <utility>             // for move

#include "Chord.h"      // for Chord
#include "Note.h"       // for Note
#include "NoteChord.h"  // for NoteChord, OCTAVE_RATIO

auto TreeNode::new_child_pointer(TreeNode *parent_pointer)
    -> std::unique_ptr<NoteChord> {
  // if parent is null, this is the root
  // the root will have no data
  if (parent_pointer == nullptr) {
    return nullptr;
  }
  auto *note_chord_pointer = parent_pointer->note_chord_pointer.get();
  if (note_chord_pointer == nullptr) {
    return std::make_unique<Chord>(instruments, default_instrument);
  }
  if (note_chord_pointer->get_level() != 1) {
    qCritical("Only chords can have children!");
  }
  return std::make_unique<Note>(instruments, default_instrument);
}

TreeNode::TreeNode(
    const std::vector<std::unique_ptr<const QString>>& instruments,
    const QString& default_instrument,
    TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      instruments(instruments),
      default_instrument(default_instrument),
      note_chord_pointer(TreeNode::new_child_pointer(parent_pointer_input)){};

auto TreeNode::copy_note_chord_pointer() const -> std::unique_ptr<NoteChord> {
  assert_not_root();
  return note_chord_pointer->copy_pointer();
}

void TreeNode::copy_children(const TreeNode &copied) {
  for (int index = 0; index < copied.child_pointers.size(); index = index + 1) {
    child_pointers.push_back(
        std::make_unique<TreeNode>(*(copied.child_pointers[index]), this));
  }
}

TreeNode::TreeNode(const TreeNode &copied, TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      instruments(copied.instruments),
      default_instrument(copied.default_instrument),
      note_chord_pointer(copied.copy_note_chord_pointer()) {
  copy_children(copied);
}

auto TreeNode::load_children(const QJsonObject &json_object) -> void {
  if (json_object.contains("children")) {
    const auto &json_children_value = json_object["children"];
    if (!(json_children_value.isArray())) {
      qCritical("Expected array!");
      return;
    }

    auto json_children = json_children_value.toArray();
    for (auto index = 0; index < json_children.size(); index = index + 1) {
      auto json_child_value = json_children.at(index);
      if (!json_child_value.isObject()) {
        TreeNode::error_not_object();
        return;
      }

      const auto &json_child = json_child_value.toObject();
      auto child_pointer =
          std::make_unique<TreeNode>(instruments, default_instrument, this);
      child_pointer->note_chord_pointer->load(json_child);
      child_pointer->load_children(json_child);
      child_pointers.push_back(std::move(child_pointer));
    }
  }
}

void TreeNode::error_not_object() { qCritical("Not an object!"); }

void TreeNode::error_row(size_t row) {
  qCritical("Invalid row %d", static_cast<int>(row));
};

auto TreeNode::is_at_row() const -> int {
  // parent_pointer is null for the root item
  // the root item is always at row 0
  assert_not_root();
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

auto TreeNode::assert_not_root() const -> void {
  if (parent_pointer == nullptr) {
    qCritical("Is root");
  }
}

auto TreeNode::get_parent() const -> TreeNode & {
  assert_not_root();
  return *parent_pointer;
}

auto TreeNode::get_child(int row) const -> TreeNode & {
  assert_child_at(row);
  return *(child_pointers[row]);
};

auto TreeNode::get_child_count() const -> size_t {
  return child_pointers.size();
};

auto TreeNode::assert_child_at(size_t position) const -> void {
  if (position < 0 || position >= get_child_count()) {
    error_row(position);
  }
}

// appending is inserting at the size
auto TreeNode::assert_insertable_at(size_t position) const -> void {
  if (position < 0 || position > get_child_count()) {
    error_row(position);
  }
}

auto TreeNode::save_children(QJsonObject &json_object) const -> void {
  QJsonArray child_array;
  auto child_count = get_child_count();
  if (child_count > 0) {
    for (auto index = 0; index < child_count; index = index + 1) {
      QJsonObject json_child;
      auto &child_node = get_child(index);
      child_node.note_chord_pointer->save(json_child);
      child_node.save_children(json_child);
      child_array.push_back(std::move(json_child));
    }
    json_object["children"] = std::move(child_array);
  }
}

auto TreeNode::get_ratio() const -> double {
  assert_not_root();
  return (1.0 * note_chord_pointer->numerator) /
         note_chord_pointer->denominator *
         pow(OCTAVE_RATIO, note_chord_pointer->octave);
}

auto TreeNode::get_level() const -> int {
  if (note_chord_pointer == nullptr) {
    return ROOT_LEVEL;
  }
  return note_chord_pointer->get_level();
}
