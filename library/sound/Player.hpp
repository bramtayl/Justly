#pragma once

#include <array>

#include "sound/FluidDriver.hpp"
#include "sound/FluidEvent.hpp"
#include "sound/FluidSequencer.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/FluidSynth.hpp"
#include "sound/PlayState.hpp"

class QWidget;
struct Program;

static const auto NUMBER_OF_MIDI_CHANNELS = 64;
static const auto NUMBER_OF_MIDI_KEYS = 128;

// what Justly last set on a channel, and how long its notes keep it busy.
// Program and pitch bend are the only channel-wide state Justly sends, so a
// note can join a channel that's still sounding if both match -- but never
// while another note holds the same key, since a note-on for a held key
// releases the first note, and the first note's note-off would then cut the
// second short. Program and bend can only change once free_time passes,
// since released voices still follow them
struct ChannelState {
  // nullptr means unclaimed since play started, so nothing matches it
  const Program* program_pointer = nullptr;
  int pitch_bend = 0;
  // when every note on the channel has finished its release
  double free_time = 0;
  // when each key's latest note-off is scheduled
  std::array<double, NUMBER_OF_MIDI_KEYS> key_free_times{};
};

[[nodiscard]] auto make_audio_driver(QWidget& parent, FluidSettings& settings,
                                     FluidSynth& synth) -> FluidDriver;

void set_destination(FluidEvent& event, fluid_seq_id_t sequencer_id);

void send_event_at(FluidSequencer& sequencer, FluidEvent& event, double time);

struct Player {
  // data
  QWidget& parent;

  // play state fields
  QList<ChannelState> channel_states;
  PlayState play_state;

  double final_time = 0;

  FluidSettings settings;

  FluidSynth synth;
  FluidEvent event;
  FluidSequencer sequencer;
  const unsigned int soundfont_id;
  FluidDriver driver;

  explicit Player(QWidget& parent_input);

  ~Player();

  NO_MOVE_COPY(Player)
};

void stop_playing(const Player& player);

[[nodiscard]] auto get_gain(const Player& player) -> double;
