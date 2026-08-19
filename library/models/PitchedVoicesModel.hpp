#pragma once

#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>

#include "models/VoicesModel.hpp"
#include "rows/PitchedVoice.hpp"

class QUndoStack;
class QVariant;
class QWidget;
struct Song;

struct PitchedVoicesModel : public VoicesModel<PitchedVoice> {
  explicit PitchedVoicesModel(QWidget& parent, QUndoStack& undo_stack,
                              Song& song_input)
      : VoicesModel<PitchedVoice>(parent, undo_stack, song_input) {}

  [[nodiscard]] auto check_cell(int column_number,
                                const QVariant& new_value) const
      -> bool override;
};
