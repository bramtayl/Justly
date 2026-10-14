#pragma once

#include <fluidsynth.h>

#include "common/helpers.hpp"

struct FluidDriver : CHandle<fluid_audio_driver_t, delete_fluid_audio_driver> {
  using CHandle::CHandle;
};
