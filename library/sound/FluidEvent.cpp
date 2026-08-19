#include "sound/FluidEvent.hpp"

#include <fluidsynth.h>
#include <fluidsynth/event.h>

#include <QtCore/QtAssert>

FluidEvent::FluidEvent() : internal_pointer(new_fluid_event()) {
  Q_ASSERT(internal_pointer != nullptr);
}

FluidEvent::~FluidEvent() { delete_fluid_event(internal_pointer); }
