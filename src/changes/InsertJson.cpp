#include "changes/InsertJson.hpp"

#include <qassert.h>  // for Q_ASSERT

#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "justly/ChordsModel.hpp"  // for ChordsModel

InsertJson::InsertJson(ChordsModel* chords_model_pointer_input,
                                       size_t first_child_number_input,
                                       nlohmann::json json_children_input,
                                       int parent_number_input,
                                       QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      json_children(std::move(json_children_input)),
      parent_number(parent_number_input) {}

auto InsertJson::undo() -> void { insert_or_remove(true); }

auto InsertJson::redo() -> void { insert_or_remove(false); }

void InsertJson::insert_or_remove(bool should_insert) {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_remove_json(first_child_number, json_children,
                                 parent_number, should_insert);
}
