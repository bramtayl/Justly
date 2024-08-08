#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <string>
#include <vector>

struct Instrument {
  const std::string instrument_name;
  const int16_t bank_number;
  const int16_t preset_number;
};

[[nodiscard]] auto instrument_is_default(const Instrument &instrument) -> bool;

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;

Q_DECLARE_METATYPE(const Instrument *);
