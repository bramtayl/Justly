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

void stop_playing(const Player& player) {
  const auto& sequencer = player.sequencer;
  const auto& event = player.event;
  fluid_sequencer_remove_events(sequencer.internal_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       ++channel_number) {
    fluid_event_all_sounds_off(event.internal_pointer, channel_number);
    fluid_sequencer_send_now(sequencer.internal_pointer,
                             event.internal_pointer);
  }
}

void set_destination(FluidEvent& event, const fluid_seq_id_t sequencer_id) {
  fluid_event_set_dest(event.internal_pointer, sequencer_id);
}

void send_event_at(FluidSequencer& sequencer, FluidEvent& event,
                   const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(fluid_sequencer_send_at(
      sequencer.internal_pointer, event.internal_pointer,
      static_cast<unsigned int>(std::round(time)), 1));
}

Player::Player(QWidget& parent_input)
    : parent(parent_input),
      channel_schedules(NUMBER_OF_MIDI_CHANNELS, 0),
      settings(NUMBER_OF_MIDI_CHANNELS,
               static_cast<int>(std::thread::hardware_concurrency()),
#ifdef __linux__
               "pulseaudio"
#else
               nullptr
#endif
               ),
      synth(settings),
      sequencer(synth),
      soundfont_id(get_soundfont_id(synth)),
      driver(make_audio_driver(parent, settings, synth)) {
  set_destination(event, sequencer.sequencer_id);
}

Player::~Player() { stop_playing(*this); }

auto get_gain(const Player& player) -> double {
  return fluid_synth_get_gain(player.synth.internal_pointer);
}
