#pragma once

#include <Gamma/AudioIO.h>    // for AudioIO
#include <Gamma/Scheduler.h>  // for Scheduler
#include <qstring.h>          // for QString, operator<

#include <map>     // for map
#include <memory>  // for unique_ptr

#include "DefaultInstrument.h"  // for DefaultInstrument
#include "Song.h"               // for DEFAULT_FREQUENCY, DEFAULT_TEMPO, DEF...
class Instrument;               // lines 11-11
class QModelIndex;              // lines 12-12
class TreeNode;                 // lines 13-13

const auto PERCENT = 100;
const auto FRAMES_PER_BUFFER = 256;
const auto SECONDS_PER_MINUTE = 60;
const auto WAIT_MILLISECONDS = 100;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto FULL_NOTE_VOLUME = 0.2F;

const DefaultInstrument DUMMY(0.0, 0.0, 0.0, 1.0);

class Player {
 public:
  float key = DEFAULT_FREQUENCY;
  float current_volume = (1.0F * DEFAULT_VOLUME_PERCENT) / PERCENT;
  float current_tempo = DEFAULT_TEMPO;
  float current_time = 0.0;

  std::map<const QString, const Instrument *> instrument_map =
      std::map<const QString, const Instrument *>{
          {"default", (const Instrument *)&DUMMY}};

  gam::Scheduler scheduler;
  std::unique_ptr<gam::AudioIO> audio_io_pointer;

  Player();

  void modulate(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> float;
  void schedule_note(const TreeNode &node);
  void play(const Song &song, const QModelIndex &first_index, int rows);
};
