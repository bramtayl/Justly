#include "sound/Player.hpp"

#include <QtWidgets/QMessageBox>
#include <thread>

#include "cell_types/Program.hpp"

auto make_audio_driver(QWidget& parent, FluidSettings& settings,
                       FluidSynth& synth) -> FluidDriver {
#ifndef NO_REALTIME_AUDIO
  auto* const audio_driver_pointer =
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

void stop_playing(const FluidSequencer& sequencer, const FluidEvent& event) {
  fluid_sequencer_remove_events(sequencer.internal_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event.internal_pointer, channel_number);
    fluid_sequencer_send_now(sequencer.internal_pointer,
                             event.internal_pointer);
  }
}

void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

void set_fluid_int(FluidSettings& settings, const char* const field,
                   const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(
      fluid_settings_setint(settings.internal_pointer, field, value));
}

void set_fluid_string(FluidSettings& settings, const char* const field,
                      const char* const value) {
  Q_ASSERT(field != nullptr);
  Q_ASSERT(value != nullptr);
  check_fluid_ok(
      fluid_settings_setstr(settings.internal_pointer, field, value));
}

void set_destination(FluidEvent& event, const fluid_seq_id_t sequencer_id) {
  fluid_event_set_dest(event.internal_pointer, sequencer_id);
}

Player::Player(QWidget& parent_input)
    : parent(parent_input),
      channel_schedules(QList<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      settings(FluidSettings(
          NUMBER_OF_MIDI_CHANNELS,
          static_cast<int>(std::thread::hardware_concurrency()),
#ifdef __linux__
          "pulseaudio"
#else
          nullptr
#endif
          )),
      synth(FluidSynth(settings)),
      sequencer(FluidSequencer(synth)),
      soundfont_id(get_soundfont_id(synth)),
      driver(make_audio_driver(parent, settings, synth)) {
  set_destination(event, sequencer.sequencer_id);
}

Player::~Player() { stop_playing(sequencer, event); }
