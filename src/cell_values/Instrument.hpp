#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <string>
#include <vector>

struct Instrument {
  const std::string name;
  const int16_t bank_number = -1;
  const int16_t preset_number = -1;
};

[[nodiscard]] auto get_instrument_pointer(
    const std::string &name) -> const Instrument *;

[[nodiscard]] auto instrument_is_default(const Instrument &instrument) -> bool;

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;

Q_DECLARE_METATYPE(const Instrument *);
