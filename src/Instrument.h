#pragma once
#include "qstring.h"

class Instrument {
 public:
  const QString name;
  const QString code;
  const int bank_number;
  const int preset_number;
  explicit Instrument(QString name_input, QString code_input,
                      int bank_number_input, int preset_number_input);
};