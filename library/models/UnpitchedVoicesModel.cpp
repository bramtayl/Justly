#include "models/UnpitchedVoicesModel.hpp"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "rows/Voice.hpp"

auto UnpitchedVoicesModel::check_cell(const int column_number,
                                      const QVariant &new_value) const -> bool {
  return check_voice_name(parent, get_rows(), static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column),
                          column_number, new_value);
}
