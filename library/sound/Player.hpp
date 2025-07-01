#pragma once

#include <QtWidgets/QMessageBox>
#include <thread>

#include "sound/FluidDriver.hpp"
#include "sound/FluidEvent.hpp"
#include "sound/FluidSequencer.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/FluidSynth.hpp"
#include "sound/PlayState.hpp"

static const auto NUMBER_OF_MIDI_CHANNELS = 64;

[[nodiscard]] static auto inline make_audio_driver(
    QWidget &parent, FluidSettings &settings,
    FluidSynth &synth) -> FluidDriver {
#ifndef NO_REALTIME_AUDIO
  auto *const audio_driver_pointer =
      new_fluid_audio_driver(settings.internal_pointer, synth.internal_pointer);
  if (audio_driver_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("Audio driver error"),
                         QObject::tr("Cannot start audio driver"));
  }
  return FluidDriver(audio_driver_pointer);
#else
  return FluidDriver(nullptr);
#endif
}

static inline void stop_playing(const FluidSequencer &sequencer,
                                const FluidEvent &event) {
  fluid_sequencer_remove_events(sequencer.internal_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event.internal_pointer, channel_number);
    fluid_sequencer_send_now(sequencer.internal_pointer,
                             event.internal_pointer);
  }
}

static void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

static inline void set_fluid_int(FluidSettings &settings,
                                 const char *const field, const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(
      fluid_settings_setint(settings.internal_pointer, field, value));
}

static inline void set_fluid_string(FluidSettings &settings,
                                    const char *const field,
                                    const char *const value) {
  Q_ASSERT(field != nullptr);
  Q_ASSERT(value != nullptr);
  check_fluid_ok(
      fluid_settings_setstr(settings.internal_pointer, field, value));
}

static inline void set_destination(FluidEvent &event,
                                   const fluid_seq_id_t sequencer_id) {
  fluid_event_set_dest(event.internal_pointer, sequencer_id);
}

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules = QList<double>(NUMBER_OF_MIDI_CHANNELS, 0);
  PlayState play_state;

  double final_time = 0;

  FluidSettings settings = []() {
    FluidSettings result;
    const auto cores = std::thread::hardware_concurrency();
    set_fluid_int(result, "synth.midi-channels", NUMBER_OF_MIDI_CHANNELS);
    if (cores > 0) {
      set_fluid_int(result, "synth.cpu-cores", static_cast<int>(cores));
    }
#ifdef __linux__
    set_fluid_string(result, "audio.driver", "pulseaudio");
#endif
    return result;
  }();

  FluidSynth synth = FluidSynth(settings);
  FluidEvent event;
  FluidSequencer sequencer = FluidSequencer(synth);
  const unsigned int soundfont_id = get_soundfont_id(synth);
  FluidDriver driver = make_audio_driver(parent, settings, synth);

  explicit Player(QWidget &parent_input) : parent(parent_input) {
    set_destination(event, sequencer.sequencer_id);
  }

  ~Player() { stop_playing(sequencer, event); }

  NO_MOVE_COPY(Player)
};
