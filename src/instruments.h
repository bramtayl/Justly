#pragma once

#include <vector>  // for vector

#include "justly/global.h"

struct Instrument;

[[nodiscard]] JUSTLY_EXPORT auto get_all_instruments() -> const std::vector<Instrument> &;
