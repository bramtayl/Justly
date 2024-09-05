#include "percussion_instrument/PercussionInstrument.hpp"

#include <QtGlobal>
#include <map>
#include <vector>

auto get_percussion_instrument_pointer(const std::string &name)
    -> const PercussionInstrument * {
  static const auto percussion_instrument_map =
      []() -> std::map<std::string, const PercussionInstrument *> {
    const std::vector<PercussionInstrument> &temp_percussion_instruments =
        get_all_percussion_instruments();
    std::map<std::string, const PercussionInstrument *> temp_map;
    for (const auto &percussion_instrument : temp_percussion_instruments) {
      temp_map[percussion_instrument.name] = &percussion_instrument;
    }
    return temp_map;
  }();
  Q_ASSERT(percussion_instrument_map.count(name) == 1);
  return percussion_instrument_map.at(name);
}

auto get_all_percussion_instruments()
    -> const std::vector<PercussionInstrument> & {
  static const std::vector<PercussionInstrument> all_percussions({
      PercussionInstrument(
          {std::string("Acoustic Bass Drum or Low Bass Drum"), 35}),
      PercussionInstrument({std::string("Acoustic Snare"), 38}),
      PercussionInstrument({std::string("Belltree"), 84}),
      PercussionInstrument({std::string("Castanets"), 85}),
      PercussionInstrument({std::string("Chinese Cymbal"), 52}),
      PercussionInstrument({std::string("Closed Hi-hat"), 42}),
      PercussionInstrument({std::string("Cowbell"), 56}),
      PercussionInstrument({std::string("Crash Cymbal 1"), 49}),
      PercussionInstrument({std::string("Crash Cymbal 2"), 57}),
      PercussionInstrument({std::string("Drum sticks"), 31}),
      PercussionInstrument(
          {std::string("Electric Bass Drum or High Bass Drum"), 36}),
      PercussionInstrument({std::string("Electric Snare or Rimshot"), 40}),
      PercussionInstrument({std::string("Hand Clap"), 39}),
      PercussionInstrument({std::string("High Floor Tom"), 43}),
      PercussionInstrument({std::string("High Q or Filter Snap"), 27}),
      PercussionInstrument({std::string("High Tom"), 50}),
      PercussionInstrument({std::string("High-Mid Tom"), 48}),
      PercussionInstrument({std::string("Jingle Bell"), 83}),
      PercussionInstrument({std::string("Low Floor Tom"), 41}),
      PercussionInstrument({std::string("Low Tom"), 45}),
      PercussionInstrument({std::string("Low-Mid Tom"), 47}),
      PercussionInstrument({std::string("Metronome Bell"), 34}),
      PercussionInstrument({std::string("Metronome Click"), 33}),
      PercussionInstrument({std::string("Mute Surdo"), 86}),
      PercussionInstrument({std::string("Open Hi-hat"), 46}),
      PercussionInstrument({std::string("Open Surdo"), 87}),
      PercussionInstrument({std::string("Pedal Hi-hat"), 44}),
      PercussionInstrument({std::string("Ride Bell"), 53}),
      PercussionInstrument({std::string("Ride Cymbal 1"), 51}),
      PercussionInstrument({std::string("Scratch Pull"), 30}),
      PercussionInstrument({std::string("Scratch Push"), 29}),
      PercussionInstrument({std::string("Shaker"), 82}),
      PercussionInstrument({std::string("Side Stick"), 37}),
      PercussionInstrument({std::string("Slap Noise"), 28}),
      PercussionInstrument({std::string("Splash Cymbal"), 55}),
      PercussionInstrument({std::string("Square Click"), 32}),
      PercussionInstrument({std::string("Tambourine"), 54}),
  });
  return all_percussions;
}
