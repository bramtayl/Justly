#include "TreeNode.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <qjsonvalue.h>

#include <algorithm>  // for max
#include <iterator>   // for move_iterator, make_move_iterator
#include <memory>     // for unique_ptr, make_unique, operator==, all...
#include <utility>

#include "Chord.h"        // for Chord
#include "Interval.h"     // for Interval
#include "Note.h"
#include "NoteChord.h"    // for NoteChord, error_level, root_level, Tree...
#include "StableIndex.h"  // for StableIndex
#include "utilities.h"    // for error_row

class Song;

auto new_child_pointer(TreeNode *parent_pointer) -> std::unique_ptr<NoteChord> {
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

TreeNode::TreeNode(TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      note_chord_pointer(new_child_pointer(parent_pointer_input)){};

auto TreeNode::get_row() const -> int {
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
  qCritical("Not a child!");
  return -1;
}

auto TreeNode::number_of_children() const -> int {
  return static_cast<int>(child_pointers.size());
};

auto TreeNode::verify_child_at(int index) const -> bool {
  if (index < 0 || index >= number_of_children()) {
    qCritical("No child at index %d!", static_cast<int>(index));
    return false;
  }
  return true;
}

// appending is inserting at the size
auto TreeNode::verify_insertable_at(int index) const -> bool {
  if (index < 0 || index > number_of_children()) {
    qCritical("Can't insert child at index %d!", static_cast<int>(index));
    return false;
  }
  return true;
}

auto TreeNode::get_ratio() const -> double {
  if (!(verify_not_root())) {
    return -1;
  }
  return (1.0 * note_chord_pointer->interval.get_ratio());
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

void TreeNode::remove_children(int first_index, int number_of_children) {
  if (!(verify_child_at(first_index) &&
        verify_child_at(first_index + number_of_children - 1))) {
    return;
  };
  child_pointers.erase(
      child_pointers.begin() + first_index,
      child_pointers.begin() + first_index + number_of_children);
}

void TreeNode::remove_save_children(
    int first_index, int number_of_children,
    std::vector<std::unique_ptr<TreeNode>> &deleted_children) {
  if (!(verify_child_at(first_index) &&
        verify_child_at(first_index + number_of_children - 1))) {
    return;
  };
  deleted_children.insert(
      deleted_children.begin(),
      std::make_move_iterator(child_pointers.begin() + first_index),
      std::make_move_iterator(child_pointers.begin() + first_index +
                              number_of_children));
  remove_children(first_index, number_of_children);
}

auto TreeNode::copy_json_children(int first_index, int number_of_children) -> QJsonArray {
  QJsonArray json_children;
  if (!(verify_child_at(first_index) &&
        verify_child_at(first_index + number_of_children - 1))) {
    return {};
  };
  for (int index = first_index; index < first_index + number_of_children;
       index = index + 1) {
    QJsonObject json_child;
    child_pointers[index] -> save_to(json_child);
    json_children.push_back(std::move(json_child));
  }
  return json_children;
}

void TreeNode::insert_empty_children(int first_index, int number_of_children) {
  if (!(verify_insertable_at(first_index))) {
    return;
  };
  for (int index = first_index; index < first_index + number_of_children;
       index = index + 1) {
    // will error if childless
    child_pointers.insert(child_pointers.begin() + index,
                          std::make_unique<TreeNode>(this));
  }
}

auto TreeNode::get_stable_index(int column) const -> StableIndex {
  auto level = get_level();
  if (level == root_level) {
    return {-1, -1, column};
  }
  if (level == chord_level) {
    return {get_row(), -1, column};
  }
  if (level == note_level) {
    return {parent_pointer->get_row(), get_row(), column};
  }
  error_level(level);
  return {-1, -1, column};
}

auto TreeNode::data(int column, int role) const -> QVariant {
  if (!(verify_not_root())) {
    return {};
  }
  return note_chord_pointer->data(column, role);
}

void TreeNode::insert_children(
    int first_index, std::vector<std::unique_ptr<TreeNode>> &insertion) {
  if (!(verify_insertable_at(first_index))) {
    return;
  }
  auto parent_level = get_level();
  for (const auto &new_child_pointer : insertion) {
    auto new_child_level = new_child_pointer->get_level();
    if (parent_level + 1 != new_child_level) {
      qCritical("Parent with level %d cannot contain children with level %d!",
                parent_level, new_child_level);
      return;
    }
  }
  child_pointers.insert(child_pointers.begin() + static_cast<int>(first_index),
                        std::make_move_iterator(insertion.begin()),
                        std::make_move_iterator(insertion.end()));
  insertion.clear();
}

// node will check for errors, so no need to check for errors here
void TreeNode::setData(int column, const QVariant &new_value) {
  if (!(verify_not_root())) {
    return;
  }
  note_chord_pointer->setData(column, new_value);
}

void TreeNode::save_to(QJsonObject& json_object) const {
  auto level = get_level();
  if (level == root_level) {
    QJsonArray chords_array;
    for (const auto &chord_node_pointer : child_pointers) {
      QJsonObject chord_object;
      chord_node_pointer -> save_to(chord_object);
      chords_array.push_back(std::move(chord_object));
    }
    json_object["chords"] = std::move(chords_array);

  } else if (level == chord_level) {
    note_chord_pointer -> save_to(json_object);
    QJsonArray note_array;
    for (const auto &note_node_pointer : child_pointers) {
      QJsonObject json_note;
      note_node_pointer -> save_to(json_note);
      note_array.push_back(json_note);
    }
    json_object["notes"] = std::move(note_array);
  } else if (level == note_level) {
    note_chord_pointer -> save_to(json_object);
  } else {
    error_level(level);
  }
}

void TreeNode::load_from(const QJsonObject& json_object) {
  auto level = get_level();
  if (level == root_level) {
    if (json_object.contains("chords")) {
      child_pointers.clear();
      insert_json_children(0, json_object["chords"].toArray());
    }
  } else if (level == chord_level) {
    note_chord_pointer -> load_from(json_object);
    if (json_object.contains("notes")) {
      child_pointers.clear();
      insert_json_children(0, json_object["notes"].toArray());
    }
  } else if (level == note_level) {
    note_chord_pointer -> load_from(json_object);
  } else {
    error_level(level);
  }
}

void TreeNode::insert_json_children(int first_index, const QJsonArray& insertion) {
  if (!(verify_insertable_at(first_index))) {
    return;
  }
  for (int offset = 0; offset < insertion.size(); offset = offset + 1) {
    auto chord_object = insertion[offset];
    auto a_new_child_pointer = std::make_unique<TreeNode>(this);
    a_new_child_pointer -> load_from(chord_object.toObject());
    child_pointers.insert(child_pointers.begin() + first_index + offset, std::move(a_new_child_pointer));
  };
}

auto TreeNode::verify_json_children(const Song& song, const QJsonArray& insertion) const -> bool {
  auto level = get_level();
  if (level == root_level) {
    for (const auto chord_value : insertion) {
      if (!(Chord::verify_json(song, chord_value))) {
        return false;
      };
    }
  } else if (level == chord_level) {
    for (const auto note_value : insertion) {
      if (!(Note::verify_json(song, note_value))) {
        return false;
      };
    }
  } else {
    error_level(level);
    return false;
  }
  return true;
}