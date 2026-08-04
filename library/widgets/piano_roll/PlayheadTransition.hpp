#pragma once

#include <cstdint>

// how position_playhead() should bring a just-started playhead onto the
// view's center, instead of snapping there instantly -- see
// position_playhead() for what each mode does
enum class PlayheadTransition : std::uint8_t {
  // no transition in progress -- every tick centers the view on the
  // playhead, as usual
  none,
  // the playhead started left of center: the view holds still and waits
  // for playback's own forward motion to carry the playhead to the
  // view's (fixed) center before switching to normal following
  waiting_to_reach_center,
  // the playhead started right of center: since playback only moves it
  // further right, the view instead eases itself from its starting
  // position to where the playhead will be once the catch-up window
  // ends, so the animated scroll and the playhead's real-time motion
  // converge together exactly at the center
  catching_up,
};
