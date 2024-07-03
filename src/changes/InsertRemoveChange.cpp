#include "changes/InsertRemoveChange.hpp"

#include <qassert.h>  // for Q_ASSERT

#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "justly/ChordsModel.hpp"  // for ChordsModel

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
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_remove_directly(first_child_number, json_children,
                                 parent_number, should_insert);
}
