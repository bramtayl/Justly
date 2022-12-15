#pragma once

#include "Gamma/AudioIO.h"    // for AudioIOData
#include "Gamma/Scheduler.h"  // for Process

class Instrument : public gam::Process<gam::AudioIOData> {
 public:
  virtual auto add(gam::Scheduler &scheduler, float start_time, float frequency,
                   float amplitude, float duration) const -> void = 0;
  void onProcess(gam::AudioIOData &audio_io) override = 0;
};
