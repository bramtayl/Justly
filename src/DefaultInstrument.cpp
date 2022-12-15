#include "DefaultInstrument.h"

#include <Gamma/AudioIO.h>    // for AudioIOData
#include <Gamma/Scheduler.h>  // for Scheduler
#include <Gamma/scl.h>        // for exp

#include <cmath>  // for exp

DefaultInstrument::DefaultInstrument(double start_time, float frequency_input,
                                     float amplitude, float duration)
    : Instrument(), frequency(frequency_input) {
  auto sustain = duration - (ATTACK_TIME + DECAY_TIME + RELEASE_TIME) + OVERLAP;
  if (sustain < MIN_SUSTAIN) {
    sustain = MIN_SUSTAIN;
  }
  dt(start_time);
  auto sustain_level = amplitude * SUSTAIN_RATIO;
  envelope.lengths(ATTACK_TIME, DECAY_TIME, sustain, RELEASE_TIME);
  envelope.levels(0, amplitude, sustain_level, sustain_level, 0);
  envelope.curve(CURVATURE);
}

auto DefaultInstrument::add(gam::Scheduler &scheduler, float start_time,
                            float frequency, float amplitude,
                            float duration) const -> void {
  scheduler.add<DefaultInstrument>(start_time, frequency, amplitude, duration);
}

void DefaultInstrument::onProcess(gam::AudioIOData &audio_io) {
  while (audio_io()) {
    auto sample_1 = oscillator() * envelope();
    audio_io.out(0) += sample_1;
    audio_io.out(1) += sample_1;
  }
  if (envelope.done()) {
    free();
  }
}
