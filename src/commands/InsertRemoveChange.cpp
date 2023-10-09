#include "commands/InsertRemoveChange.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "models/ChordsModel.h"  // for ChordsModel

InsertRemoveChange::InsertRemoveChange(
    gsl::not_null<ChordsModel *> chords_model_pointer_input,
    int first_child_number_input, nlohmann::json insertion_input,
    const StableIndex &stable_parent_index_input,
    bool is_insert_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      insertion(std::move(insertion_input)),
      stable_parent_index(stable_parent_index_input),
      is_insert(is_insert_input) {}

void InsertRemoveChange::insert_if(bool should_insert) {
  if (should_insert) {
    chords_model_pointer->insert_json_children_directly(first_child_number, insertion,
                                             stable_parent_index);
  } else {
    chords_model_pointer->remove_rows_directly(first_child_number,
                                             static_cast<int>(insertion.size()),
                                             stable_parent_index);
  }
}

// remove_save will check for errors, so no need to check here
auto InsertRemoveChange::redo() -> void {
  insert_if(is_insert);
}

auto InsertRemoveChange::undo() -> void {
  insert_if(!is_insert);
}
