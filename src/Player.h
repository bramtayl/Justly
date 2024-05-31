#pragma once

#include <iosfwd>             // for stringstream
#include <memory>             // for allocator, unique_ptr
#include <string>             // for string
#include <vector>             // for vector

#include <fluidsynth.h>

struct Chord;
struct Instrument;
struct Note;
struct Song;

class Player {
  double current_key;
  double current_volume;
  double current_tempo;
  double current_time;
  const Instrument *current_instrument_pointer;
  const Song *song_pointer;
  fluid_synth_t *synth_pointer = nullptr;
  fluid_sequencer_t *sequencer_pointer;
  fluid_audio_driver_t *audio_driver_pointer = nullptr;
  int16_t synth_id = -1;
  int soundfont_id = -1;


  void initialize();
  void update_with_chord(const Chord *);
  void move_time(const Chord *);
  void play_notes(const Chord* chord_pointer, int first_note_index = 0, int number_of_notes = -1) const;

  [[nodiscard]] auto get_beat_duration() const -> double;

 public:
  explicit Player(Song *song_pointer);
  ~Player();
  void set_up(const std::string &output_file = "");
  void play_selection(int first_child_number, int number_of_children,
                    int chord_number);
  void play_chords(int first_chord_index = 0, int number_of_chords = -1);
  void write_song(const std::string &output_file);
  void stop_playing();
  void kill();

  // prevent moving and copying;
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};
