#include "src/InsertRemoveChange.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "src/ChordsModel.h"  // for ChordsModel

InsertRemoveChange::InsertRemoveChange(ChordsModel* chords_model_pointer_input,
                                       int first_child_number_input,
                                       nlohmann::json json_children_input,
                                       int chord_number_input,
                                       bool is_insert_input,
                                       QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      json_children(std::move(json_children_input)),
      chord_number(chord_number_input),
      is_insert(is_insert_input) {}

// remove_save will check for errors, so no need to check here
auto InsertRemoveChange::redo() -> void { insert_or_remove(is_insert); }

auto InsertRemoveChange::undo() -> void { insert_or_remove(!is_insert); }
