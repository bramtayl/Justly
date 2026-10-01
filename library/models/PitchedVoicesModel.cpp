#include "models/PitchedVoicesModel.hpp"

#include "actions/RenameVoice.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "other/Song.hpp"
#include "rows/PitchedNote.hpp"

auto PitchedVoicesModel::check_cell(const int column_number,
                                    const QVariant& new_value) const -> bool {
  return check_voice_name(
      parent, get_rows(),
      static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column),
      column_number, new_value);
}

auto PitchedVoicesModel::make_set_cell(const QModelIndex& index,
                                       const QVariant& new_value)
    -> QUndoCommand* {
  if (index.column() ==
      static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)) {
    return new RenameVoice<  // NOLINT(cppcoreguidelines-owning-memory)
        PitchedVoice, PitchedNote>(*this, index,
                                   variant_to<QString>(new_value));
  }
  return VoicesModel<PitchedVoice>::make_set_cell(index, new_value);
}
