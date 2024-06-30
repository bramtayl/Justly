#pragma once

#include <fluidsynth.h>        // for new_fluid_event, new_fluid_se...
#include <fluidsynth/types.h>  // for fluid_audio_driver_t, fluid_e...

#include <cstddef>  // for size_t
#include <string>   // for string
#include <vector>   // for vector

#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT, NO_MOVE_COPY

struct Chord;
struct Instrument;
struct Song;

auto JUSTLY_EXPORT get_default_driver() -> std::string;

class JUSTLY_EXPORT Player {
  Song* song_pointer;
  std::vector<unsigned int> channel_schedules;

  double starting_time = 0;
  double current_time = 0;

  double current_key = 0;
  double current_volume = 0;
  double current_tempo = 0;
  const Instrument* current_instrument_pointer = nullptr;

  fluid_settings_t* settings_pointer = new_fluid_settings();
  unsigned int soundfont_id = 0;
  fluid_synth_t* synth_pointer = nullptr;
  fluid_event_t* event_pointer = new_fluid_event();
  fluid_sequencer_t* sequencer_pointer = new_fluid_sequencer2(0);
  fluid_seq_id_t sequencer_id = -1;
  fluid_audio_driver_t* audio_driver_pointer = nullptr;

  [[nodiscard]] auto beat_time() const -> double;
  void initialize_play();
  [[nodiscard]] auto has_real_time() const -> bool;
  void modulate(const Chord& chord);
  auto play_notes(size_t chord_index, const Chord& chord,
                  size_t first_note_index, size_t number_of_notes)
      -> unsigned int;
  auto play_chords(size_t first_chord_index, size_t number_of_chords,
                   int wait_frames = 0) -> unsigned int;

 public:
  explicit Player(Song* song_pointer);
  ~Player();
  NO_MOVE_COPY(Player)

  void start_real_time(const std::string& driver = get_default_driver());

  [[nodiscard]] auto get_playback_volume() const -> float;
  void set_playback_volume(float value);

  void play(int parent_number, size_t first_child_number,
            size_t number_of_children);
  void stop_playing();

  void export_to_file(const std::string& output_file);
};