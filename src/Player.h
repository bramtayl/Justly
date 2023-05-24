#pragma once

#include <stddef.h>  // for size_t

#include <fstream>  // for ofstream
#include <string>   // for string, basic_string, allocator
#include <vector>   // for vector

#include "CsoundData.h"  // for CsoundData
#include "Song.h"        // for DEFAULT_FREQUENCY, DEFAULT_TEMPO, DEFAULT_VO...
class QModelIndex;       // lines 8-8
class TreeNode;          // lines 9-9

const auto PERCENT = 100;
const auto FRAMES_PER_BUFFER = 256;
const auto SECONDS_PER_MINUTE = 60;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto FULL_NOTE_VOLUME = 0.2;

class Player {
 public:
  std::string csound_file = "";
  double key = DEFAULT_FREQUENCY;
  double current_volume = (1.0 * DEFAULT_VOLUME_PERCENT) / PERCENT;
  double current_tempo = DEFAULT_TEMPO;
  double current_time = 0.0;

  CsoundData csound_data;

  const std::vector<std::string> instruments = {
      "BandedWG", "BeeThree", "BlowBotl", "BlowHole", "Bowed",    "Brass",
      "Clarinet", "Drummer",  "Flute",    "FMVoices", "HevyMetl", "Mandolin",
      "ModalBar", "Moog",     "PercFlut", "Plucked",  "Resonate", "Rhodey",
      "Saxofony", "Shakers",  "Simple",   "Sitar",    "StifKarp", "TubeBell",
      "VoicForm", "Whistle",  "Wurley"};

  static void add_instrument(std::ofstream &csound_io,
                             const std::string &instrument);

  void modulate(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> double;
  void schedule_note(std::ofstream &csound_io, const TreeNode &node) const;
  void play(const Song &song, const QModelIndex &first_index, size_t rows);
};
