#include "justly/Instrument.hpp"

#include <algorithm>  // for find_if
#include <string>     // for string, operator==
#include <utility>    // for move
#include <vector>     // for vector

#include "song/instruments.hpp"  // for get_all_instruments

Instrument::Instrument(std::string name_input, int16_t bank_number_input,
                       int16_t preset_number_input)
    : instrument_name(std::move(name_input)),
      bank_number(bank_number_input),
      preset_number(preset_number_input) {}

auto Instrument::is_default() const -> bool {
  return !instrument_name.empty();
}

auto get_instrument_pointer(const std::string &instrument_name) -> const Instrument * {
  const auto &instruments = get_all_instruments();
  for (const auto& instrument : instruments) {
    if (instrument.instrument_name == instrument_name) {
      return &instrument;
    }
  }
  Q_ASSERT(false);
  return nullptr;
}

