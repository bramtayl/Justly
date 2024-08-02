#pragma once

#include <QByteArray>
#include <QMetaType>
#include <cstdint>
#include <string>

#include "justly/JUSTLY_EXPORT.hpp"

class JUSTLY_EXPORT Instrument {
public:
  const std::string instrument_name;
  const int16_t bank_number;
  const int16_t preset_number;

  explicit Instrument(std::string name_input = "",
                      int16_t bank_number_input = -1,
                      int16_t preset_number_input = -1);
};

[[nodiscard]] JUSTLY_EXPORT auto get_instrument_pointer(
    const std::string &instrument_name) -> const Instrument *;

Q_DECLARE_METATYPE(const Instrument *);
