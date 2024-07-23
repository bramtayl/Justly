#pragma once

#include <cstddef>

const auto NOTE_CHORD_COLUMNS = 7;

const auto PERCENT = 100;
const auto MAX_GAIN = 10;

const auto MIN_STARTING_KEY = 60;
const auto MAX_STARTING_KEY = 440;

const auto MAX_STARTING_VOLUME = 100;

const auto MIN_STARTING_TEMPO = 25;
const auto MAX_STARTING_TEMPO = 200;

const auto MAX_NUMERATOR = 199;
const auto MAX_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto MAX_OCTAVE = 9;

const auto SMALL_SPACING = 2;

auto to_size_t(int input) -> size_t;
auto to_unsigned(int input) -> unsigned int;
