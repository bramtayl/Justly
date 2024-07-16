#include "changes/InsertJsonNotes.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <utility>

#include "justly/ChordsModel.hpp"

InsertJsonNotes::InsertJsonNotes(ChordsModel *chords_model_pointer_input,
                       size_t first_child_number_input,
                       nlohmann::json json_children_input,
                       size_t parent_number_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      json_children(std::move(json_children_input)),
      parent_number(parent_number_input) {}

auto InsertJsonNotes::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_notes(first_child_number,
                                        json_children.size(), parent_number);
}

auto InsertJsonNotes::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_json_notes(first_child_number, json_children, parent_number);
}
