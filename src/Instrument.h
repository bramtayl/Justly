#pragma once
#include "qstring.h"

class Instrument {
 public:
  const QString display_name;
  const QString code_name;
  const int bank_number;
  const int preset_number;
  explicit Instrument(QString display_name_input, QString code_name_input,
                      int bank_number_input, int preset_number_input);
};