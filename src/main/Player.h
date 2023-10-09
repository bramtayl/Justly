#pragma once

#include <qstring.h>

#include <csound/csound.hpp>        // for Csound
#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread
#include <gsl/pointers>
#include <memory>  // for unique_ptr

#include "metatypes/Instrument.h"

class QTextStream;
class Song;
class TreeNode;

class Player : public Csound {
 private:
  double current_key = 0.0;
  double current_volume = 0.0;
  double current_tempo = 0.0;
  double current_time = 0.0;
  std::unique_ptr<CsoundPerformanceThread> performer_pointer = nullptr;
  gsl::not_null<const Instrument *> current_instrument_pointer =
      &(Instrument::get_instrument_by_name(""));
  gsl::not_null<Song *> song_pointer;

  void initialize_song();
  void update_with_chord(const TreeNode &node);
  void move_time(const TreeNode &node);
  void write_note(QTextStream *output_stream_pointer,
                  const TreeNode &node) const;

  [[nodiscard]] auto get_beat_duration() const -> double;

 public:
  explicit Player(gsl::not_null<Song *> song_pointer,
                  const QString &output_file = "");
  ~Player() override;

  void write_song();
  void write_chords(int first_child_number, int number_of_children,
                    const TreeNode &parent_node);
  void stop_playing();
  [[nodiscard]] auto has_real_time() const -> bool;

  // prevent moving and copying;
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};
