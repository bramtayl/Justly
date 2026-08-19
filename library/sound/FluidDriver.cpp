#include "sound/FluidDriver.hpp"

#include <fluidsynth.h>
#include <fluidsynth/audio.h>

void FluidDriver::reset() {
  if (internal_pointer != nullptr) {
    delete_fluid_audio_driver(internal_pointer);
  }
  internal_pointer = nullptr;
}

auto FluidDriver::operator=(FluidDriver &&other) noexcept -> FluidDriver & {
  if (this != &other) {
    reset();
    internal_pointer = other.internal_pointer;
    other.internal_pointer = nullptr;
  }
  return *this;
}

FluidDriver::FluidDriver(FluidDriver &&other) noexcept
    : internal_pointer(other.internal_pointer) {
  other.internal_pointer = nullptr;
}

FluidDriver::~FluidDriver() { reset(); }
