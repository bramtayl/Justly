#pragma once

#include <QtWidgets/QWidget>
#include <fluidsynth.h>
#include <thread>

#include "PlayState.hpp"

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules = QList<double>(NUMBER_OF_MIDI_CHANNELS, 0);
  PlayState play_state;

  double final_time = 0;

  fluid_settings_t &settings = []() -> fluid_settings_t & {
    fluid_settings_t &result = get_reference(new_fluid_settings());
    const auto cores = std::thread::hardware_concurrency();
    set_fluid_int(result, "synth.midi-channels", NUMBER_OF_MIDI_CHANNELS);
    if (cores > 0) {
      set_fluid_int(result, "synth.cpu-cores", static_cast<int>(cores));
    }
#ifdef __linux__
    fluid_settings_setstr(&result, "audio.driver", "pulseaudio");
#endif
    return result;
  }();
  fluid_event_t &event = get_reference(new_fluid_event());
  fluid_sequencer_t &sequencer = get_reference(new_fluid_sequencer2(0));
  fluid_synth_t &synth = get_reference(new_fluid_synth(&settings));
  const unsigned int soundfont_id = get_soundfont_id(synth);
  const fluid_seq_id_t sequencer_id =
      fluid_sequencer_register_fluidsynth(&sequencer, &synth);

  fluid_audio_driver_t *audio_driver_pointer =
      make_audio_driver(parent, settings, synth);

  // methods
  explicit Player(QWidget &parent_input) : parent(parent_input) {
    fluid_event_set_dest(&event, sequencer_id);
  }

  ~Player() {
    stop_playing(sequencer, event);
    maybe_delete_audio_driver(audio_driver_pointer);
    fluid_sequencer_unregister_client(&sequencer, sequencer_id);
    delete_fluid_sequencer(&sequencer);
    delete_fluid_event(&event);
    delete_fluid_synth(&synth);
    delete_fluid_settings(&settings);
  }

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};
