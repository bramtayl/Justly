#pragma once

#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>

#include "models/VoicesModel.hpp"
#include "rows/UnpitchedVoice.hpp"

class QUndoStack;
class QWidget;
struct Song;

struct UnpitchedVoicesModel : public VoicesModel<UnpitchedVoice> {
  explicit UnpitchedVoicesModel(QWidget &parent, QUndoStack &undo_stack,
                                Song &song_input)
      : VoicesModel<UnpitchedVoice>(parent, undo_stack, song_input) {}

  [[nodiscard]] auto check_cell( int column_number,
                                const QVariant &new_value) const
      -> bool override;
};
