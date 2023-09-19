#pragma once

#include <string>
#include <vector>

#include "qstring.h"

class Instrument {
 public:
  const QString instrument_name;
  const int bank_number;
  const int preset_number;
  const int instument_id;
  explicit Instrument(QString name_input,
                      int bank_number_input, int preset_number_input,
                      int instrument_id_input);
  [[nodiscard]] static auto get_all_instruments() -> const std::vector<Instrument> &;
  [[nodiscard]] static auto get_all_instrument_names() -> const std::vector<std::string>&;
};