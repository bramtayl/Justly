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
  InsertRemoveChange(ChordsModel*, int, nlohmann::json, int, bool,
                     QUndoCommand* = nullptr);

  void undo() override;
  void redo() override;

  void insert_or_remove(bool should_insert) {
    if (should_insert) {
      chords_model_pointer->insert(first_child_number, json_children,
                                   parent_number);
    } else {
      chords_model_pointer->remove(first_child_number,
                                   static_cast<int>(json_children.size()),
                                   parent_number);
    }
  }
};
