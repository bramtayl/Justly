#pragma once

#include "PercussionInstrument.hpp"

struct PlayState {
  double current_time = 0;

  const Program *current_instrument_pointer = nullptr;
  PercussionInstrument current_percussion_instrument;
  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;
};

