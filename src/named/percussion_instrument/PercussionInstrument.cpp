#include "named/percussion_instrument/PercussionInstrument.hpp"

#include <QList>

PercussionInstrument::PercussionInstrument(const char *name,
                                           short midi_number_input)
    : Named(name), midi_number(midi_number_input) {}

auto PercussionInstrument::get_all_nameds()
    -> const QList<PercussionInstrument> & {
  static const QList<PercussionInstrument> all_percussions({
      PercussionInstrument("Acoustic or Low Bass Drum", 35),
      PercussionInstrument("Acoustic Snare", 38),
      PercussionInstrument("Belltree", 84),
      PercussionInstrument("Castanets", 85),
      PercussionInstrument("Chinese Cymbal", 52),
      PercussionInstrument("Closed Hi-hat", 42),
      PercussionInstrument("Cowbell", 56),
      PercussionInstrument("Crash Cymbal 1", 49),
      PercussionInstrument("Crash Cymbal 2", 57),
      PercussionInstrument("Drum sticks", 31),
      PercussionInstrument("Electric or High Bass Drum", 36),
      PercussionInstrument("Electric Snare or Rimshot", 40),
      PercussionInstrument("Hand Clap", 39),
      PercussionInstrument("High Floor Tom", 43),
      PercussionInstrument("High Q or Filter Snap", 27),
      PercussionInstrument("High Tom", 50),
      PercussionInstrument("High-Mid Tom", 48),
      PercussionInstrument("Jingle Bell", 83),
      PercussionInstrument("Low Floor Tom", 41),
      PercussionInstrument("Low Tom", 45),
      PercussionInstrument("Low-Mid Tom", 47),
      PercussionInstrument("Metronome Bell", 34),
      PercussionInstrument("Metronome Click", 33),
      PercussionInstrument("Mute Surdo", 86),
      PercussionInstrument("Open Hi-hat", 46),
      PercussionInstrument("Open Surdo", 87),
      PercussionInstrument("Pedal Hi-hat", 44),
      PercussionInstrument("Ride Bell", 53),
      PercussionInstrument("Ride Cymbal 1", 51),
      PercussionInstrument("Scratch Pull", 30),
      PercussionInstrument("Scratch Push", 29),
      PercussionInstrument("Shaker", 82),
      PercussionInstrument("Side Stick", 37),
      PercussionInstrument("Slap Noise", 28),
      PercussionInstrument("Splash Cymbal", 55),
      PercussionInstrument("Square Click", 32),
      PercussionInstrument("Tambourine", 54),
  });
  return all_percussions;
}
