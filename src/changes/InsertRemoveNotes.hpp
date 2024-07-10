#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "justly/ChordsModel.hpp"  // for ChordsModel
#include "justly/Note.hpp"         // for Note

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
