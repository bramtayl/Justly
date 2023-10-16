#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <gsl/pointers>
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json

#include "utilities/SongIndex.h"  // for SongIndex

class ChordsModel;  // lines 12-12

class InsertRemoveChange : public QUndoCommand {
 private:
  gsl::not_null<ChordsModel *> chords_model_pointer;
  int first_child_number;
  nlohmann::json insertion;
  SongIndex parent_song_index;
  bool is_insert;

 public:
  InsertRemoveChange(gsl::not_null<ChordsModel *>,
                     int,
                     nlohmann::json,
                     const SongIndex &,
                     bool,
                     QUndoCommand * = nullptr);
  void insert_if(bool);

  void undo() override;
  void redo() override;
};
