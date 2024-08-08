#include "justly/get_instrument_pointer.hpp"

#include <QtGlobal>
#include <map>
#include <string>
#include <vector>

#include "cell_values/Instrument.hpp"

auto get_instrument_pointer(const std::string &instrument_name)
    -> const Instrument * {
  static const auto instrument_map = []() {
    const std::vector<Instrument> &instruments = get_all_instruments();
    std::map<std::string, const Instrument *> instrument_map;
    for (const auto &instrument : instruments) {
      instrument_map[instrument.instrument_name] = &instrument;
    }
    return instrument_map;
  }();
  Q_ASSERT(instrument_map.count(instrument_name) == 1);
  return instrument_map.at(instrument_name);
}
