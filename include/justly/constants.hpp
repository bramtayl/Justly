#pragma once

#include "qcolor.h"

const auto NON_DEFAULT_COLOR = QColor(Qt::black);
const auto DEFAULT_COLOR = QColor(Qt::lightGray);

const auto NOTE_CHORD_COLUMNS = 7;

const auto MIN_NUMERATOR = 1;
const auto DEFAULT_NUMERATOR = 1;
const auto MAX_NUMERATOR = 199;

const auto MIN_DENOMINATOR = 1;
const auto DEFAULT_DENOMINATOR = 1;
const auto MAX_DENOMINATOR = 199;

const auto MIN_OCTAVE = -9;
const auto DEFAULT_OCTAVE = 0;
const auto MAX_OCTAVE = 9;