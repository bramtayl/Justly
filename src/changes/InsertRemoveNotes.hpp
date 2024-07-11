#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/ChordsModel.hpp"
#include "justly/Note.hpp"

class InsertRemoveNotes : public QUndoCommand {
  ChordsModel* const chords_model_pointer;
  const size_t first_child_number;
  const std::vector<Note> notes;
  const int parent_number;
  const bool is_insert;

 public:
  InsertRemoveNotes(ChordsModel* chords_model_pointer_input,
                    size_t first_child_number_input,
                    const std::vector<Note>& notes_input,
                    int parent_number_input, bool is_insert_input,
                    QUndoCommand* parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;

  void insert_or_remove(bool should_insert);
};
