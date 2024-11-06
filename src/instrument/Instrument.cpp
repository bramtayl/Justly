#include "instrument/Instrument.hpp"

#include <utility>

Instrument::Instrument(QString name, short bank_number, short preset_number)
    : AbstractInstrument(std::move(name), bank_number, preset_number) {}

auto Instrument::get_all_nameds() -> const QList<Instrument> & {
  static const auto all_instruments = fill_instruments<Instrument>(false);
  return all_instruments;
}
