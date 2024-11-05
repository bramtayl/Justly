#include "instrument/Instrument.hpp"

#include <QList>
#include <utility>

Instrument::Instrument(QString name, short bank_number, short preset_number)
    : AbstractInstrument(std::move(name), bank_number, preset_number) {}

auto Instrument::get_all_nameds() -> const QList<Instrument> & {
  static const auto all_instruments = []() {
    QList<Instrument> temp_instruments;
    fill_instruments(temp_instruments, false);
    return temp_instruments;
  }();

  return all_instruments;
}
