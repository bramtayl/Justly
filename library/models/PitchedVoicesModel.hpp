#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>

#include "column_numbers/PitchedVoiceColumn.hpp"
#include "models/VoicesModel.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Voice.hpp"

class QUndoStack;
class QWidget;
struct Song;

struct PitchedVoicesModel : public VoicesModel<PitchedVoice> {
  explicit PitchedVoicesModel(QWidget &parent, QUndoStack &undo_stack,
                              Song &song_input)
      : VoicesModel<PitchedVoice>(parent, undo_stack, song_input) {}

  [[nodiscard]] auto check_cell(const int column_number,
                                const QVariant &new_value) const
      -> bool override;
};
