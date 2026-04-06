#pragma once

#include "models/UndoRowsModel.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice> struct VoicesModel : public UndoRowsModel<SubVoice> {
  explicit VoicesModel(QUndoStack &undo_stack, Song &song_input)
      : UndoRowsModel<SubVoice>(undo_stack, song_input) {
  }
  int created_voices = 0;
};
