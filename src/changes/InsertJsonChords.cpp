#include "changes/InsertJsonChords.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <utility>

#include "justly/ChordsModel.hpp"

InsertJsonChords::InsertJsonChords(ChordsModel *chords_model_pointer_input,
                       size_t first_chord_number_input,
                       nlohmann::json json_chords_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_chord_number(first_chord_number_input),
      json_chords(std::move(json_chords_input)) {}

auto InsertJsonChords::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_chords(first_chord_number,
                                        json_chords.size());
}

auto InsertJsonChords::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_json_chords(first_chord_number, json_chords);
}
