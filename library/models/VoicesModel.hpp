#pragma once

#include "actions/RenameVoice.hpp"
#include "models/UndoRowsModel.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/UnpitchedNote.hpp"
#include "rows/UnpitchedVoice.hpp"

// SubNote is the kind of note that names these voices, so renaming a voice
// can rename its notes too
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct VoicesModel : public UndoRowsModel<SubVoice> {
  QWidget& parent;
  int created_voices = 0;
  explicit VoicesModel(QWidget& parent_input, QUndoStack& undo_stack,
                       Song& song_input)
      : UndoRowsModel<SubVoice>(undo_stack, song_input), parent(parent_input) {}

  [[nodiscard]] auto check_cell(const int column_number,
                                const QVariant& new_value) const
      -> bool override {
    return check_voice_name(parent, this->get_rows(), column_number, new_value);
  }

  [[nodiscard]] auto make_set_cell(const QModelIndex& index,
                                   const QVariant& new_value)
      -> QUndoCommand* override {
    if (index.column() == SubVoice::get_name_column()) {
      return new RenameVoice<  // NOLINT(cppcoreguidelines-owning-memory)
          SubVoice, SubNote>(*this, index.row(),
                             variant_to<QString>(new_value));
    }
    return UndoRowsModel<SubVoice>::make_set_cell(index, new_value);
  }
};

using PitchedVoicesModel = VoicesModel<PitchedVoice, PitchedNote>;
using UnpitchedVoicesModel = VoicesModel<UnpitchedVoice, UnpitchedNote>;
