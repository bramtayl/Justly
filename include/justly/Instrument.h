#pragma once

#include <string>  // for string
#include <vector>  // for vector

struct Instrument {
  std::string instrument_name;
  int bank_number;
  int preset_number;
  int instrument_id;
  explicit Instrument(std::string name_input = "", int bank_number_input = -1,
                      int preset_number_input = -1,
                      int instrument_id_input = -1);
};

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;
[[nodiscard]] auto get_instrument(const std::string &instrument_name)
    -> const Instrument &;
