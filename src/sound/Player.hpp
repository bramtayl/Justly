#pragma once

#include <QtWidgets/QMessageBox>
#include <fluidsynth.h>
#include <thread>

#include "sound/PlayState.hpp"

static const auto NUMBER_OF_MIDI_CHANNELS = 64;

[[nodiscard]] static auto inline make_audio_driver(
    QWidget &parent, fluid_settings_t &settings,
    fluid_synth_t &synth) -> fluid_audio_driver_t * {
#ifndef NO_REALTIME_AUDIO
  auto *const audio_driver_pointer = new_fluid_audio_driver(&settings, &synth);
  if (audio_driver_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("Audio driver error"),
                         QObject::tr("Cannot start audio driver"));
  }
  return audio_driver_pointer;
#else
  return nullptr;
#endif
}

static inline void
maybe_delete_audio_driver(fluid_audio_driver_t *const audio_driver_pointer) {
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }
}

static inline void stop_playing(fluid_sequencer_t &sequencer,
                                fluid_event_t &event) {
  fluid_sequencer_remove_events(&sequencer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(&event, channel_number);
    fluid_sequencer_send_now(&sequencer, &event);
  }
}

static void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static inline void set_fluid_int(fluid_settings_t &settings,
                                 const char *const field, const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(fluid_settings_setint(&settings, field, value));
}

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
