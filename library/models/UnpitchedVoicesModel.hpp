#pragma once

#include "models/VoicesModel.hpp"
#include "rows/UnpitchedVoice.hpp"

struct UnpitchedVoicesModel : public VoicesModel<UnpitchedVoice> {
  explicit UnpitchedVoicesModel(QWidget &parent, QUndoStack &undo_stack,
                                Song &song_input)
      : VoicesModel<UnpitchedVoice>(parent, undo_stack, song_input) {}

  [[nodiscard]] auto check_cell(const int column_number,
                                const QVariant &new_value) const
      -> bool override {
    return check_voice_name(parent, get_rows(), unpitched_voice_name_column,
                            column_number, new_value);
  }
};
