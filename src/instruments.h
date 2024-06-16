#pragma once

#include <vector>  // for vector

struct Instrument;

[[nodiscard]] auto get_all_instruments() -> const std::vector<Instrument> &;
