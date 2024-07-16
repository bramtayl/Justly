#include "changes/InsertJsonNotes.hpp"

#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <utility>

#include "justly/ChordsModel.hpp"

InsertJsonNotes::InsertJsonNotes(ChordsModel *chords_model_pointer_input,
                       size_t first_note_number_input,
                       nlohmann::json json_notes_input,
                       size_t chord_number_input,
                       QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_note_number(first_note_number_input),
      json_notes(std::move(json_notes_input)),
      chord_number(chord_number_input) {}

auto InsertJsonNotes::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_notes(first_note_number,
                                        json_notes.size(), chord_number);
}

auto InsertJsonNotes::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_json_notes(first_note_number, json_notes, chord_number);
}
