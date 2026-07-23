#pragma once

#include "models/UndoRowsModel.hpp"
#include "rows/Voice.hpp"

class QUndoStack;
class QWidget;
struct Song;

template <VoiceInterface SubVoice> struct VoicesModel : public UndoRowsModel<SubVoice> {
  QWidget& parent;
  int created_voices = 0;
  explicit VoicesModel(QWidget& parent_input, QUndoStack &undo_stack, Song &song_input)
      : UndoRowsModel<SubVoice>(undo_stack, song_input), parent(parent_input) {
  }
};
