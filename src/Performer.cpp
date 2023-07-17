#include "Performer.h"

class Csound;

Performer::Performer(Csound* player_pointer_input)
    : CsoundPerformanceThread(player_pointer_input) {
};

Performer::~Performer() {
  Stop();
  Join();
}

void Performer::stop_playing() {
  Pause();
  SetScoreOffsetSeconds(0);
  InputMessage("i \"clear_events\" 0 0");
}
