#pragma once

#include <string>  // for string, allocator

struct Instrument {
  std::string instrument_name;
  int bank_number;
  int preset_number;
  explicit Instrument(std::string name_input = "", int bank_number_input = -1,
                      int preset_number_input = -1);
};

[[nodiscard]] auto get_instrument(const std::string &instrument_name)
    -> const Instrument &;
