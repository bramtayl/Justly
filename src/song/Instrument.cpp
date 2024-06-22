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

auto get_instrument(const std::string &instrument_name) -> const Instrument & {
  const auto &instruments = get_all_instruments();
  return *std::find_if(instruments.cbegin(), instruments.cend(),
                       [instrument_name](const Instrument &instrument) {
                         return instrument.instrument_name == instrument_name;
                       });
}