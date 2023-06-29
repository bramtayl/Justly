#include "Instrument.h"

#include <qstring.h>  // for QString

#include <utility>  // for move

Instrument::Instrument(QString name_input, QString code_input,
                       int bank_number_input, int preset_number_input)
    : name(std::move(name_input)),
      code(std::move(code_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input) {}