#pragma once

#include <vector>

class Instrument;

[[nodiscard]] auto instrument_is_default(const Instrument &instrument) -> bool;

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;
