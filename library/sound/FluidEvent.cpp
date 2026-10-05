#include "sound/FluidEvent.hpp"

FluidEvent::FluidEvent() : CHandle(new_fluid_event()) {
  Q_ASSERT(internal_pointer != nullptr);
}
