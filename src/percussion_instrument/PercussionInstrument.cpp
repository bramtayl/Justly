#include "percussion_instrument/PercussionInstrument.hpp"

#include <QtGlobal>
#include <QMap>
#include <QList>

auto get_all_percussion_instruments()
    -> const QList<PercussionInstrument> & {
  static const QList<PercussionInstrument> all_percussions({
      PercussionInstrument(
          {QString("Acoustic Bass Drum or Low Bass Drum"), 35}),
      PercussionInstrument({QString("Acoustic Snare"), 38}),
      PercussionInstrument({QString("Belltree"), 84}),
      PercussionInstrument({QString("Castanets"), 85}),
      PercussionInstrument({QString("Chinese Cymbal"), 52}),
      PercussionInstrument({QString("Closed Hi-hat"), 42}),
      PercussionInstrument({QString("Cowbell"), 56}),
      PercussionInstrument({QString("Crash Cymbal 1"), 49}),
      PercussionInstrument({QString("Crash Cymbal 2"), 57}),
      PercussionInstrument({QString("Drum sticks"), 31}),
      PercussionInstrument(
          {QString("Electric Bass Drum or High Bass Drum"), 36}),
      PercussionInstrument({QString("Electric Snare or Rimshot"), 40}),
      PercussionInstrument({QString("Hand Clap"), 39}),
      PercussionInstrument({QString("High Floor Tom"), 43}),
      PercussionInstrument({QString("High Q or Filter Snap"), 27}),
      PercussionInstrument({QString("High Tom"), 50}),
      PercussionInstrument({QString("High-Mid Tom"), 48}),
      PercussionInstrument({QString("Jingle Bell"), 83}),
      PercussionInstrument({QString("Low Floor Tom"), 41}),
      PercussionInstrument({QString("Low Tom"), 45}),
      PercussionInstrument({QString("Low-Mid Tom"), 47}),
      PercussionInstrument({QString("Metronome Bell"), 34}),
      PercussionInstrument({QString("Metronome Click"), 33}),
      PercussionInstrument({QString("Mute Surdo"), 86}),
      PercussionInstrument({QString("Open Hi-hat"), 46}),
      PercussionInstrument({QString("Open Surdo"), 87}),
      PercussionInstrument({QString("Pedal Hi-hat"), 44}),
      PercussionInstrument({QString("Ride Bell"), 53}),
      PercussionInstrument({QString("Ride Cymbal 1"), 51}),
      PercussionInstrument({QString("Scratch Pull"), 30}),
      PercussionInstrument({QString("Scratch Push"), 29}),
      PercussionInstrument({QString("Shaker"), 82}),
      PercussionInstrument({QString("Side Stick"), 37}),
      PercussionInstrument({QString("Slap Noise"), 28}),
      PercussionInstrument({QString("Splash Cymbal"), 55}),
      PercussionInstrument({QString("Square Click"), 32}),
      PercussionInstrument({QString("Tambourine"), 54}),
  });
  return all_percussions;
}

auto get_percussion_instrument_pointer(const QString &name)
    -> const PercussionInstrument * {
  static const auto percussion_instrument_map =
      []() -> QMap<QString, const PercussionInstrument *> {
    const QList<PercussionInstrument> &temp_percussion_instruments =
        get_all_percussion_instruments();
    QMap<QString, const PercussionInstrument *> temp_map;
    for (const auto &percussion_instrument : temp_percussion_instruments) {
      temp_map[percussion_instrument.name] = &percussion_instrument;
    }
    return temp_map;
  }();
  Q_ASSERT(percussion_instrument_map.count(name) == 1);
  return percussion_instrument_map.value(name);
}
