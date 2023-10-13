#pragma once

#include <qstring.h>

#include <csound/csound.hpp>        // for Csound
#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread
#include <gsl/pointers>
#include <iosfwd>  // for stringstream
#include <memory>  // for unique_ptr

#include "metatypes/Instrument.h"
#include "notechord/Chord.h"
#include "notechord/Note.h"

class Song;

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
  void update_with_chord(const Chord& chord);
  void move_time(const Chord& chord);
  void write_note(std::stringstream *output_stream_pointer,
                  const Note& note) const;

  [[nodiscard]] auto get_beat_duration() const -> double;

 public:
  explicit Player(gsl::not_null<Song *> song_pointer,
                  const QString &output_file = "");
  ~Player() override;

  void write_song();
  void write_chords(int first_child_number, int number_of_children,
                    int chord_number);
  void stop_playing();
  [[nodiscard]] auto has_real_time() const -> bool;

  // prevent moving and copying;
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};
