#include "Instrument.h"

#include <qstring.h>  // for QString

#include <utility>  // for move

Instrument::Instrument(QString display_name_input, QString code_name_input,
                       int bank_number_input, int preset_number_input)
    : display_name(std::move(display_name_input)),
      code_name(std::move(code_name_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input) {}