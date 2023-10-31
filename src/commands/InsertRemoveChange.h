#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

class ChordsModel;  // lines 12-12

class InsertRemoveChange : public QUndoCommand {
 private:
  ChordsModel* chords_model_pointer;
  int first_child_number;
  nlohmann::json json_children;
  int chord_number;
  bool is_insert;

 public:
  InsertRemoveChange(ChordsModel*, int, nlohmann::json, int, bool,
                     QUndoCommand* = nullptr);
  void insert_if(bool);

  void undo() override;
  void redo() override;
};
