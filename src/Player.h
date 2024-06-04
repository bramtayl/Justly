#pragma once

#include <fluidsynth.h>        // for new_fluid_event, new_fluid_sequencer2
#include <fluidsynth/types.h>  // for fluid_audio_driver_t, fluid_event_t

#include <string>  // for string

struct Chord;
struct Instrument;
struct Song;

class Player {
  double current_key = 0;
  double current_volume = 0;
  double current_tempo = 0;
  double current_time = 0;
  const Instrument *current_instrument_pointer;
  const Song *song_pointer;
  fluid_event_t *event_pointer = new_fluid_event();
  fluid_settings_t *settings_pointer = new_fluid_settings();
  fluid_synth_t *synth_pointer = nullptr;
  fluid_sequencer_t *sequencer_pointer = new_fluid_sequencer2(0);
  fluid_audio_driver_t *audio_driver_pointer = nullptr;
  fluid_seq_id_t sequencer_id = -1;
  int soundfont_id = -1;

  void initialize();
  void update_with_chord(const Chord *);
  void play_notes(const Chord *chord_pointer, int first_note_index = 0,
                  int number_of_notes = -1) const;

  [[nodiscard]] auto get_beat_duration() const -> double;

 public:
  explicit Player(Song *song_pointer);
  ~Player();
  void start_real_time();
  void play_selection(int first_child_number, int number_of_children,
                      int chord_number);
  void play_chords(int first_chord_index = 0, int number_of_chords = -1);
  void write_song(const std::string &output_file);
  void stop_playing();

  // prevent moving and copying;
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};
