#include "justly/Instrument.hpp"

#include <QtGlobal>
#include <string>
#include <utility>
#include <vector>

#include "cell_values/instruments.hpp"

Instrument::Instrument(std::string name_input, int16_t bank_number_input,
                       int16_t preset_number_input)
    : instrument_name(std::move(name_input)), bank_number(bank_number_input),
      preset_number(preset_number_input) {}

auto get_instrument_pointer(const std::string &instrument_name)
    -> const Instrument * {
  const std::vector<Instrument> &instruments = get_all_instruments();
  for (const auto &instrument : instruments) {
    if (instrument.instrument_name == instrument_name) {
      return &instrument;
    }
  }
  Q_ASSERT(false);
  return nullptr;
}
