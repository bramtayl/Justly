#pragma once

#include <string>

#include "justly/JUSTLY_EXPORT.hpp"

struct Instrument;

[[nodiscard]] JUSTLY_EXPORT auto get_instrument_pointer(
    const std::string &instrument_name) -> const Instrument *;
