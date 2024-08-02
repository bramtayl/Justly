#pragma once

#include <string>
#include <vector>

class Instrument;

[[nodiscard]] auto instrument_is_default(const Instrument &instrument) -> bool;

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;

[[nodiscard]] auto get_instrument_names() -> const std::vector<std::string> &;