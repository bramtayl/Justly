#pragma once

#include <QUndoStack>
#include <cstddef>

struct NotesModel;
struct Instrument;

struct SetNoteInstrument : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const size_t note_number;
  const Instrument *const old_instrument_pointer;
  const Instrument *const new_instrument_pointer;

  explicit SetNoteInstrument(NotesModel *notes_model_pointer_input,
                             size_t note_number_input,
                             const Instrument *old_instrument_pointer_input,
                             const Instrument *new_instrument_pointer_input,
                             QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
