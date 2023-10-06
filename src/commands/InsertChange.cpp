#include "commands/InsertChange.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

InsertChange::InsertChange(gsl::not_null<ChordsModel *> chords_model_pointer_input,
                           int first_index_input,
                           nlohmann::json insertion_input,
                           const QModelIndex &parent_index_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_index(first_index_input),
      insertion(std::move(insertion_input)),
      stable_parent_index(chords_model_pointer->get_stable_index(
          parent_index_input)) {}

// remove_save will check for errors, so no need to check here
auto InsertChange::redo() -> void {
  chords_model_pointer->insert_json_children(
      first_index, insertion,
      chords_model_pointer->get_unstable_index(stable_parent_index));
}

auto InsertChange::undo() -> void {
  chords_model_pointer->removeRows(first_index, static_cast<int>(insertion.size()),
                          chords_model_pointer->get_unstable_index(stable_parent_index));
}
