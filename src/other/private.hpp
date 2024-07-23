#pragma once

#include <cstddef>

const auto NOTE_CHORD_COLUMNS = 7;

const auto MAX_NUMERATOR = 199;
const auto MAX_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto MAX_OCTAVE = 9;

const auto SMALL_SPACING = 2;

auto to_size_t(int input) -> size_t;
auto to_unsigned(int input) -> unsigned int;
