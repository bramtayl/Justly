#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <cstddef>                // for size_t
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "justly/ChordsModel.hpp"

class InsertJson : public QUndoCommand {
  ChordsModel* const chords_model_pointer;
  const size_t first_child_number;
  const nlohmann::json json_children;
  const int parent_number;

 public:
  InsertJson(ChordsModel* chords_model_pointer_input,
             size_t first_child_number_input,
             nlohmann::json json_children_input, int parent_number_input,
             QUndoCommand* parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;

  void insert_or_remove(bool should_insert);
};
