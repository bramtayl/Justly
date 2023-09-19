#pragma once

#include <qstring.h>

#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread
#include <csound/csound.hpp>        // for Csound
#include <memory>                   // for unique_ptr

class QTextStream;
class Song;
class TreeNode;

const auto HALFSTEPS_PER_OCTAVE = 12;
const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto PERCENT = 100;

class Player : public Csound {
 public:
  std::unique_ptr<CsoundPerformanceThread> performer_pointer = nullptr;
  double current_key = 0.0;
  double current_volume = 0.0;
  double current_tempo = 0.0;
  double current_time = 0.0;
  QString current_instrument;
  Song &song;

  explicit Player(Song &song, const QString &output_file = "");
  ~Player() override;

  void initialize_song();
  void update_with_chord(const TreeNode &node);
  void move_time(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> double;
  void write_note(QTextStream &output_stream, const TreeNode &node) const;
  void write_song();
  void write_chords(int first_index, int number_of_children,
                    const TreeNode &parent_node);
  void stop_playing() const;

  // prevent moving and copying;
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};