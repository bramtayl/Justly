#include "models/UnpitchedVoicesModel.hpp"

#include "actions/RenameVoice.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "other/Song.hpp"
#include "rows/UnpitchedNote.hpp"

auto UnpitchedVoicesModel::check_cell(const int column_number,
                                      const QVariant& new_value) const -> bool {
  return check_voice_name(
      parent, get_rows(),
      static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column),
      column_number, new_value);
}

auto UnpitchedVoicesModel::make_set_cell(const QModelIndex& index,
                                         const QVariant& new_value)
    -> QUndoCommand* {
  if (index.column() ==
      static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)) {
    return new RenameVoice<  // NOLINT(cppcoreguidelines-owning-memory)
        UnpitchedVoice, UnpitchedNote>(*this, index.row(),
                                       variant_to<QString>(new_value));
  }
  return VoicesModel<UnpitchedVoice>::make_set_cell(index, new_value);
}
