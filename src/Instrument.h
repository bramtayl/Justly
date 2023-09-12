#pragma once
#include <vector>

#include "qstring.h"

class Instrument {
 public:
  const QString name;
  const QString code;
  const int bank_number;
  const int preset_number;
  const int id;
  explicit Instrument(QString name_input, QString code_input,
                      int bank_number_input, int preset_number_input,
                      int id_input);
  [[nodiscard]] static auto get_all_instruments() -> std::vector<Instrument> &;
  [[nodiscard]] static auto get_all_instrument_names() -> QString &;
};