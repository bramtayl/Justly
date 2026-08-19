#pragma once

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtCompare>
#include <QtCore/QtSwap>
#include <cmath>
#include <cstdlib>
#include <tuple>

#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "sound/PlayState.hpp"

static const auto C_0_MIDI = 12;
static const auto CENTS_PER_HALFSTEP = 100;
static const auto DEFAULT_STARTING_MIDI = MIDDLE_C_MIDI;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

struct Song {
  double starting_key = midi_number_to_frequency(DEFAULT_STARTING_MIDI);
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
  QList<PitchedVoice> pitched_voices;
  QList<UnpitchedVoice> unpitched_voices;
};

[[nodiscard]] auto get_octave_degree(int midi_interval)
    -> std::tuple<int, int>;

void initialize_playstate(const Song &song, PlayState &play_state,
                           double current_time);

[[nodiscard]] auto get_play_state_at_chord(const Song &song,
                                           const int chord_number) -> PlayState;

[[nodiscard]] auto get_note_name(const int closest_midi)
    -> QString;

void add_frequency_to_stream(QTextStream &stream,
                             const double frequency);

void add_timing_to_stream(QTextStream &stream,
                          const PlayState &play_state,
                          const double velocity,
                          const double beats_double);
