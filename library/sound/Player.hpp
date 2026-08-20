#pragma once


#include "sound/FluidDriver.hpp"
#include "sound/FluidEvent.hpp"
#include "sound/FluidSequencer.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/FluidSynth.hpp"
#include "sound/PlayState.hpp"

class QWidget;
struct Program;

static const auto NUMBER_OF_MIDI_CHANNELS = 64;

[[nodiscard]] auto make_audio_driver(QWidget& parent, FluidSettings& settings,
                                     FluidSynth& synth) -> FluidDriver;

void stop_playing(const FluidSequencer& sequencer, const FluidEvent& event);

void check_fluid_ok(int fluid_result);

void set_fluid_int(FluidSettings& settings, const char* field, int value);

void set_fluid_string(FluidSettings& settings, const char* field,
                      const char* value);

void set_destination(FluidEvent& event, fluid_seq_id_t sequencer_id);

struct Player {
  // data
  QWidget& parent;

  // play state fields
  QList<double> channel_schedules;
  // percussion programs don't send pitch bend and (per MS_Basic.sf3)
  // have no breath-controller modulators, so unlike pitched notes, a channel
  // can safely be shared by overlapping notes of the same percussion program
  // -- once a program claims a channel here, play_note never lets
  // channel_schedules make it eligible for reuse by anything else, so
  // switching a channel's program mid-decay (the actual source of glitches)
  // can't happen for percussion at all
  QHash<const Program*, int> percussion_channels;
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
