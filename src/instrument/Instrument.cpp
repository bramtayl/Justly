#include "instrument/Instrument.hpp"
#include "named/Named.hpp"

#include <QList>
#include <QtGlobal>

class QStringListModel;

Instrument::Instrument(const QString &name, short bank_number_input,
                       short preset_number_input)
    : Named({name}), bank_number(bank_number_input),
      preset_number(preset_number_input) {}

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

auto get_instrument_schema() -> nlohmann::json {
  return nlohmann::json({{"type", "string"},
                         {"description", "the instrument"},
                         {"enum", get_names(get_all_instruments())}});
};

[[nodiscard]] auto get_instrument_names_model() -> QStringListModel & {
  static auto instrument_names_model = get_list_model(get_all_instruments());
  return instrument_names_model;
}
