#pragma once

#include <QUndoStack>
#include <cstddef>

#include "interval/Interval.hpp"

struct NotesModel;

struct SetNoteInterval : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const size_t note_number;
  const Interval old_interval;
  const Interval new_interval;

  explicit SetNoteInterval(NotesModel *notes_model_pointer_input,
                           size_t note_number_input,
                           const Interval &old_interval_input,
                           const Interval &new_interval_input,
                           QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
