#include "instrument/Instrument.hpp"

#include <QList>
#include <QtGlobal>
#include <utility>

Instrument::Instrument(QString name, short bank_number, short preset_number)
    : AbstractInstrument(std::move(name), bank_number, preset_number) {}

auto variant_to_instrument(const QVariant &variant) -> const Instrument * {
  Q_ASSERT(variant.canConvert<const Instrument *>());
  return variant.value<const Instrument *>();
}

auto get_all_instruments() -> const QList<Instrument> & {
  static const auto all_instruments = []() {
    QList<Instrument> temp_instruments;
    fill_instruments(temp_instruments, false);
    return temp_instruments;
  }();

  return all_instruments;
}

