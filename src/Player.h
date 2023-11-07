#pragma once

#include <csound/csound.hpp>  // for Csound
#include <iosfwd>             // for stringstream
#include <memory>             // for allocator, unique_ptr
#include <string>             // for string

class CsoundPerformanceThread;
struct Chord;
struct Instrument;
struct Note;
struct Song;

class Player : public Csound {
  double current_key;
  double current_volume;
  double current_tempo;
  double current_time;
  std::unique_ptr<CsoundPerformanceThread> performer_pointer;
  const Instrument *current_instrument_pointer;
  const Song *song_pointer;

  void initialize();
  void update_with_chord(const Chord *);
  void move_time(const Chord *);
  void write_note(std::stringstream *, const Note *) const;

  [[nodiscard]] auto get_beat_duration() const -> double;

 public:
  explicit Player(Song *song_pointer, const std::string &output_file = "");
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
