#pragma once

#include <cstdint>  // for int16_t
#include <string>  // for string, allocator

struct Instrument {
  std::string instrument_name;
  int16_t bank_number;
  int16_t preset_number;
  explicit Instrument(std::string name_input = "", int16_t bank_number_input = -1,
                      int16_t preset_number_input = -1);
};

[[nodiscard]] auto get_instrument(const std::string &instrument_name)
    -> const Instrument &;
