#include "changes/InsertJsonChords.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <utility>

#include "justly/ChordsModel.hpp"

InsertJsonChords::InsertJsonChords(ChordsModel *chords_model_pointer_input,
                       size_t first_child_number_input,
                       nlohmann::json json_children_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      json_children(std::move(json_children_input)) {}

auto InsertJsonChords::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_chords(first_child_number,
                                        json_children.size());
}

auto InsertJsonChords::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_json_chords(first_child_number, json_children);
}
