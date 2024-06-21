#include "changes/InsertRemoveChange.hpp"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

InsertRemoveChange::InsertRemoveChange(ChordsModel* chords_model_pointer_input,
                                       int first_child_number_input,
                                       nlohmann::json json_children_input,
                                       int parent_number_input,
                                       bool is_insert_input,
                                       QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      json_children(std::move(json_children_input)),
      parent_number(parent_number_input),
      is_insert(is_insert_input) {}

auto InsertRemoveChange::undo() -> void { insert_or_remove(!is_insert); }

auto InsertRemoveChange::redo() -> void { insert_or_remove(is_insert); }

void InsertRemoveChange::insert_or_remove(bool should_insert) {
  if (should_insert) {
    chords_model_pointer->insert(first_child_number, json_children,
                                 parent_number);
  } else {
    chords_model_pointer->remove(first_child_number,
                                 static_cast<int>(json_children.size()),
                                 parent_number);
  }
}
