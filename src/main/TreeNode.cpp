#include "main/TreeNode.h"

#include <qglobal.h>   // for qCritical
#include <qvariant.h>  // for QVariant

#include <algorithm>              // for copy, max
#include <cstddef>                // for size_t
#include <iterator>               // for move_iterator, make_move_iterator
#include <map>                    // for operator!=
#include <memory>                 // for unique_ptr, make_unique, operator==
#include <nlohmann/json.hpp>      // for basic_json, basic_json<>::object_t
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "metatypes/Interval.h"     // for Interval
#include "notechord/Chord.h"        // for Chord
#include "notechord/Note.h"         // for Note
#include "notechord/NoteChord.h"    // for NoteChord, chord_level, note_level
#include "utilities/StableIndex.h"  // for StableIndex

auto new_child_pointer(TreeNode *parent_pointer) -> std::unique_ptr<NoteChord> {
  // if parent is null, this is the root
  // the root will have no data
  if (parent_pointer == nullptr) {
    return nullptr;
  }
  auto &parent = *(parent_pointer);
  if (parent.is_root()) {
    return std::make_unique<Chord>();
  }
  return parent.get_note_chord().new_child_pointer();
}

TreeNode::TreeNode(TreeNode *parent_pointer_input)
    : parent_pointer(parent_pointer_input),
      note_chord_pointer(new_child_pointer(parent_pointer_input)) {}

auto TreeNode::get_row() const -> int {
  const auto &siblings = get_const_parent().child_pointers;
  for (size_t index = 0; index < siblings.size(); index = index + 1) {
    if (this == siblings[index].get()) {
      return static_cast<int>(index);
    }
  }
  qCritical("Not a child!");
  return -1;
}

auto TreeNode::number_of_children() const -> int {
  return static_cast<int>(child_pointers.size());
}

auto TreeNode::get_ratio() const -> double {
  return get_const_note_chord().interval.get_ratio();
}

auto TreeNode::get_level() const -> TreeLevel {
  if (is_root()) {
    return root_level;
  }
  return get_const_note_chord().get_level();
}

auto TreeNode::is_root() const -> bool { return note_chord_pointer == nullptr; }

void TreeNode::remove_children(int first_index, int number_of_children) {
  child_pointers.erase(
      child_pointers.begin() + first_index,
      child_pointers.begin() + first_index + number_of_children);
}

void TreeNode::remove_save_children(
    int first_index, int number_of_children,
    std::vector<std::unique_ptr<TreeNode>> &deleted_children) {
  deleted_children.insert(
      deleted_children.begin(),
      std::make_move_iterator(child_pointers.begin() + first_index),
      std::make_move_iterator(child_pointers.begin() + first_index +
                              number_of_children));
  remove_children(first_index, number_of_children);
}

auto TreeNode::copy_json_children(int first_index, int number_of_children)
    -> nlohmann::json {
  nlohmann::json json_children;
  for (int index = first_index; index < first_index + number_of_children;
       index = index + 1) {
    nlohmann::json json_child = nlohmann::json::object();
    child_pointers[index]->save_to(json_child);
    json_children.push_back(std::move(json_child));
  }
  return json_children;
}

void TreeNode::insert_empty_children(int first_index, int number_of_children) {
  for (int index = first_index; index < first_index + number_of_children;
       index = index + 1) {
    // will error if childless
    child_pointers.insert(child_pointers.begin() + index,
                          std::make_unique<TreeNode>(this));
  }
}

auto TreeNode::get_stable_index(int column) const -> StableIndex {
  auto level = get_level();
  if (level == note_level) {
    return {get_const_parent().get_row(), get_row(), column};
  }
  if (level == chord_level) {
    return {get_row(), -1, column};
  }
  // root level
  return {-1, -1, column};
}

auto TreeNode::data(int column, int role) const -> QVariant {
  return get_const_note_chord().data(column, role);
}

void TreeNode::insert_children(
    int first_index, std::vector<std::unique_ptr<TreeNode>> &insertion) {
  auto parent_level = get_level();
  for (const auto &new_child_pointer : insertion) {
    auto new_child_level = new_child_pointer->get_level();
    if (parent_level + 1 != new_child_level) {
      qCritical("Parent with level %d cannot contain children with level %d!",
                parent_level, new_child_level);
      return;
    }
  }
  child_pointers.insert(child_pointers.begin() + first_index,
                        std::make_move_iterator(insertion.begin()),
                        std::make_move_iterator(insertion.end()));
  insertion.clear();
}

// node will check for errors, so no need to check for errors here
void TreeNode::setData(int column, const QVariant &new_value) {
  get_note_chord().setData(column, new_value);
}

void TreeNode::save_to(nlohmann::json &json_object) const {
  auto level = get_level();
  if (level == note_level) {
    get_const_note_chord().save_to(json_object);
    return;
  }
  if (level == chord_level) {
    get_const_note_chord().save_to(json_object);
    if (!(child_pointers.empty())) {
      nlohmann::json note_array;
      for (const auto &note_node_pointer : child_pointers) {
        nlohmann::json json_note = nlohmann::json::object();
        note_node_pointer->save_to(json_note);
        note_array.push_back(json_note);
      }
      json_object["notes"] = std::move(note_array);
    }
    return;
  }
  // root level
  nlohmann::json chords_array;
  for (const auto &chord_node_pointer : child_pointers) {
    nlohmann::json chord_object = nlohmann::json::object();
    chord_node_pointer->save_to(chord_object);
    chords_array.push_back(std::move(chord_object));
  }
  json_object["chords"] = std::move(chords_array);
}

void TreeNode::load_from(const nlohmann::json &json_object) {
  auto level = get_level();
  if (level == note_level) {
    get_note_chord().load_from(json_object);
    return;
  }
  if (level == chord_level) {
    get_note_chord().load_from(json_object);
    if (json_object.contains("notes")) {
      child_pointers.clear();
      insert_json_children(0, json_object["notes"]);
    }
    return;
  }
  // root level
  if (json_object.contains("chords")) {
    child_pointers.clear();
    insert_json_children(0, json_object["chords"]);
  }
}

void TreeNode::insert_json_children(int first_index,
                                    const nlohmann::json &insertion) {
  for (int offset = 0; offset < static_cast<int>(insertion.size());
       offset = offset + 1) {
    const auto &chord_object = insertion[offset];
    auto a_new_child_pointer = std::make_unique<TreeNode>(this);
    a_new_child_pointer->load_from(chord_object);
    child_pointers.insert(child_pointers.begin() + first_index + offset,
                          std::move(a_new_child_pointer));
  }
}

auto TreeNode::verify_json_children(const nlohmann::json &paste_json) const
    -> bool {
  if (is_root()) {
    return Chord::verify_json_items(paste_json);
  }
  return Note::verify_json_items(paste_json);
}

auto TreeNode::get_const_parent() const -> const TreeNode & {
  return *parent_pointer;
}

auto TreeNode::get_note_chord() -> NoteChord & { return *note_chord_pointer; }

auto TreeNode::get_const_note_chord() const -> const NoteChord & {
  return *note_chord_pointer;
}