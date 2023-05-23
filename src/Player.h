#pragma once

#include <fstream>       // for ofstream
#include <string>        // for allocator, string
#include "CsoundData.h"  // for CsoundData
#include "Song.h"        // for DEFAULT_FREQUENCY, DEFAULT_TEMPO, DEFAULT_VO...
class QModelIndex;  // lines 6-6
class TreeNode;  // lines 7-7

const auto PERCENT = 100;
const auto FRAMES_PER_BUFFER = 256;
const auto SECONDS_PER_MINUTE = 60;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto FULL_NOTE_VOLUME = 0.2;

class Player {
 public:

  std::string csound_file = "";
  double key = DEFAULT_FREQUENCY;
  double current_volume = (1.0F * DEFAULT_VOLUME_PERCENT) / PERCENT;
  double current_tempo = DEFAULT_TEMPO;
  double current_time = 0.0;

  CsoundData csound_data;

  static void add_instrument(std::ofstream& csound_io, const std::string& instrument);

  void modulate(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> double;
  void schedule_note(std::ofstream &csound_io, const TreeNode &node) const;
  void play(const Song &song, const QModelIndex &first_index, int rows);
};
