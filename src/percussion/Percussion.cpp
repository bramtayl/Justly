#include "percussion/Percussion.hpp"

#include <QtGlobal>
#include <map>
#include <vector>

auto get_percussion_pointer(const std::string &name) -> const Percussion * {
  static const auto percussion_map = []() {
    const std::vector<Percussion> &temp_percussions = get_all_percussions();
    std::map<std::string, const Percussion *> temp_map;
    for (const auto &percussion : temp_percussions) {
      temp_map[percussion.name] = &percussion;
    }
    return temp_map;
  }();
  Q_ASSERT(percussion_map.count(name) == 1);
  return percussion_map.at(name);
}

auto get_all_percussions() -> const std::vector<Percussion> & {
  static const std::vector<Percussion> all_percussions({
      Percussion({std::string("Acoustic Bass Drum or Low Bass Drum"), 35}),
      Percussion({std::string("Acoustic Snare"), 38}),
      Percussion({std::string("Belltree"), 84}),
      Percussion({std::string("Castanets"), 85}),
      Percussion({std::string("Chinese Cymbal"), 52}),
      Percussion({std::string("Closed Hi-hat"), 42}),
      Percussion({std::string("Cowbell"), 56}),
      Percussion({std::string("Crash Cymbal 1"), 49}),
      Percussion({std::string("Crash Cymbal 2"), 57}),
      Percussion({std::string("Drum sticks"), 31}),
      Percussion({std::string("Electric Bass Drum or High Bass Drum"), 36}),
      Percussion({std::string("Electric Snare or Rimshot"), 40}),
      Percussion({std::string("Hand Clap"), 39}),
      Percussion({std::string("High Floor Tom"), 43}),
      Percussion({std::string("High Q or Filter Snap"), 27}),
      Percussion({std::string("High Tom"), 50}),
      Percussion({std::string("High-Mid Tom"), 48}),
      Percussion({std::string("Jingle Bell"), 83}),
      Percussion({std::string("Low Floor Tom"), 41}),
      Percussion({std::string("Low Tom"), 45}),
      Percussion({std::string("Low-Mid Tom"), 47}),
      Percussion({std::string("Metronome Bell"), 34}),
      Percussion({std::string("Metronome Click"), 33}),
      Percussion({std::string("Mute Surdo"), 86}),
      Percussion({std::string("Open Hi-hat"), 46}),
      Percussion({std::string("Open Surdo"), 87}),
      Percussion({std::string("Pedal Hi-hat"), 44}),
      Percussion({std::string("Ride Bell"), 53}),
      Percussion({std::string("Ride Cymbal 1"), 51}),
      Percussion({std::string("Scratch Pull"), 30}),
      Percussion({std::string("Scratch Push"), 29}),
      Percussion({std::string("Shaker"), 82}),
      Percussion({std::string("Side Stick"), 37}),
      Percussion({std::string("Slap Noise"), 28}),
      Percussion({std::string("Splash Cymbal"), 55}),
      Percussion({std::string("Square Click"), 32}),
      Percussion({std::string("Tambourine"), 54}),
  });
  return all_percussions;
}
