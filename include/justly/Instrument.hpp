#pragma once

#include <qmetatype.h>  // for qRegisterMetaType, qRegisterNormaliz...

#include <cstdint>  // for int16_t
#include <string>   // for string, allocator

#include "justly/public_constants.hpp"

struct JUSTLY_EXPORT Instrument {
  std::string instrument_name;
  int16_t bank_number;
  int16_t preset_number;
  explicit Instrument(std::string name_input = "",
                      int16_t bank_number_input = -1,
                      int16_t preset_number_input = -1);

  [[nodiscard]] auto is_default() const -> bool;
};

[[nodiscard]] JUSTLY_EXPORT auto get_instrument_pointer(
    const std::string &instrument_name) -> const Instrument *;

[[nodiscard]] JUSTLY_EXPORT auto get_all_instruments()
    -> const std::vector<Instrument> &;

Q_DECLARE_METATYPE(const Instrument *);
