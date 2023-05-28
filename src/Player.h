#pragma once

#include <stddef.h>  // for size_t

#include <string>  // for allocator, string

#include "CsoundData.h"  // for CsoundData
#include "Song.h"        // for DEFAULT_FREQUENCY, DEFAULT_TEMPO, DEFAULT_VO...
class QModelIndex;       // lines 10-10
class QTextStream;
class TreeNode;  // lines 11-11
#include <qtemporaryfile.h>  // for QTemporaryFile

const auto PERCENT = 100;
const auto FRAMES_PER_BUFFER = 256;
const auto SECONDS_PER_MINUTE = 60;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto FULL_NOTE_VOLUME = 0.2;

class Player {
 public:
  double key = DEFAULT_FREQUENCY;
  double current_volume = (1.0 * DEFAULT_VOLUME_PERCENT) / PERCENT;
  double current_tempo = DEFAULT_TEMPO;
  double current_time = 0.0;

  CsoundData csound_data;

  QTemporaryFile score_file;

  explicit Player(const QString orchestra_file);
  void modulate(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> double;
  void schedule_note(QTextStream &csound_io, const TreeNode &node) const;
  void play(const Song &song, const QModelIndex &first_index, size_t rows);
};
