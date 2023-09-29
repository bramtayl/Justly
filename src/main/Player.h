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

const auto HALFSTEPS_PER_OCTAVE = 12;
const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto PERCENT = 100;

class Player : public Csound {
 private:
  double current_key = 0.0;
  double current_volume = 0.0;
  double current_tempo = 0.0;
  double current_time = 0.0;
  gsl::not_null<const Instrument *> current_instrument_pointer =
      &(Instrument::get_instrument_by_name(""));
  gsl::not_null<Song *> song_pointer;

  void initialize_song();
  void update_with_chord(const TreeNode &node);
  void move_time(const TreeNode &node);
  void write_note(QTextStream &output_stream, const TreeNode &node) const;

  [[nodiscard]] auto get_beat_duration() const -> double;
  auto get_performer() -> CsoundPerformanceThread &;

 public:
  std::unique_ptr<CsoundPerformanceThread> performer_pointer = nullptr;

  explicit Player(gsl::not_null<Song *> song_pointer,
                  const QString &output_file = "");
  ~Player() override;

  void write_song();
  void write_chords(int first_index, int number_of_children,
                    const TreeNode &parent_node);
  void stop_playing();
  [[nodiscard]] auto get_current_instrument() const -> const Instrument &;
  void set_current_instrument(const Instrument &new_instrument);

  // prevent moving and copying;
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};
