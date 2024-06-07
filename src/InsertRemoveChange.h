#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <nlohmann/json.hpp>  // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <utility>                // for move

#include "justly/macros.h"
#include "src/ChordsModel.h"  // for ChordsModel
#include "src/InsertRemoveChange.h"

class InsertRemoveChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  int first_child_number;
  nlohmann::json json_children;
  int chord_number;
  bool is_insert;

 public:
  InsertRemoveChange(ChordsModel*, int, nlohmann::json, int, bool,
                     QUndoCommand* = nullptr);
  NO_MOVE_COPY(InsertRemoveChange);

  void undo() override;
  void redo() override;

  inline void insert_or_remove(bool should_insert) {
    if (should_insert) {
      chords_model_pointer->insert_json_children_directly(
          first_child_number, json_children, chord_number);
    } else {
      chords_model_pointer->remove_children_directly(
          first_child_number, static_cast<int>(json_children.size()),
          chord_number);
    }
  }
};
