#include "commands/InsertChange.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "main/Editor.h"         // for Editor
#include "models/ChordsModel.h"  // for ChordsModel

class QModelIndex;

InsertChange::InsertChange(gsl::not_null<Editor *> editor_pointer_input,
                           int first_index_input,
                           nlohmann::json insertion_input,
                           const QModelIndex &parent_index_input,
                           QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      editor_pointer(editor_pointer_input),
      first_index(first_index_input),
      insertion(std::move(insertion_input)),
      stable_parent_index(
          editor_pointer->chords_model_pointer->get_stable_index(
              parent_index_input)) {}

// remove_save will check for errors, so no need to check here
auto InsertChange::redo() -> void {
  editor_pointer->register_changed();
  editor_pointer->chords_model_pointer->insert_json_children(
      first_index, insertion,
      editor_pointer->chords_model_pointer->get_unstable_index(
          stable_parent_index));
}

auto InsertChange::undo() -> void {
  editor_pointer->chords_model_pointer->removeRows(
      first_index, static_cast<int>(insertion.size()),
      editor_pointer->chords_model_pointer->get_unstable_index(
          stable_parent_index));
}