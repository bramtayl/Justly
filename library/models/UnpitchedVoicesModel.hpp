#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>

#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "models/VoicesModel.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "rows/Voice.hpp"

class QUndoStack;
class QWidget;
struct Song;

struct UnpitchedVoicesModel : public VoicesModel<UnpitchedVoice> {
  explicit UnpitchedVoicesModel(QWidget &parent, QUndoStack &undo_stack,
                                Song &song_input)
      : VoicesModel<UnpitchedVoice>(parent, undo_stack, song_input) {}

  [[nodiscard]] auto check_cell(const int column_number,
                                const QVariant &new_value) const
      -> bool override {
    return check_voice_name(parent, get_rows(), static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column),
                            column_number, new_value);
  }
};
