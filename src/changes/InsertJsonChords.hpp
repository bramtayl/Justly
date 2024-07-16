#pragma once

#include <QUndoStack>
#include <cstddef>
#include <nlohmann/json.hpp>

#include "justly/ChordsModel.hpp"

class InsertJsonChords : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t first_chord_number;
  const nlohmann::json json_chords;

public:
  InsertJsonChords(ChordsModel *chords_model_pointer_input,
             size_t first_chord_number_input,
             nlohmann::json json_chords_input,
             QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;

  void insert_or_remove(bool should_insert);
};
