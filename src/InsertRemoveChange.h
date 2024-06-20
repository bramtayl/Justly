#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "src/ChordsModel.h"  // for ChordsModel

class InsertRemoveChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  int first_child_number;
  nlohmann::json json_children;
  int parent_number;
  bool is_insert;

 public:
  InsertRemoveChange(ChordsModel* chords_model_pointer_input,
                     int first_child_number_input,
                     nlohmann::json json_children_input,
                     int parent_number_input, bool is_insert_input,
                     QUndoCommand* parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;

  void insert_or_remove(bool should_insert) {
    if (should_insert) {
      chords_model_pointer->insert(first_child_number, json_children,
                                   parent_number);
    } else {
      chords_model_pointer->remove(first_child_number, json_children.size(),
                                   parent_number);
    }
  }
};
