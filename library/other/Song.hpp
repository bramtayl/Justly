#pragma once

#include "rows/Chord.hpp"

static const auto C_0_MIDI = 12;
static const auto CENTS_PER_HALFSTEP = 100;
static const auto DEFAULT_STARTING_MIDI = 57;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

struct Song {
  double starting_key = midi_number_to_frequency(DEFAULT_STARTING_MIDI);
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
};

[[nodiscard]] static auto inline get_octave_degree(int midi_interval)
    -> std::tuple<int, int> {
  const int octave =
      to_int(std::floor((1.0 * midi_interval) / HALFSTEPS_PER_OCTAVE));
  return std::make_tuple(octave, midi_interval - octave * HALFSTEPS_PER_OCTAVE);
}

static void initialize_playstate(const Song &song, PlayState &play_state,
                                 double current_time) {
  play_state.current_instrument_pointer = nullptr;
  play_state.current_percussion_instrument = PercussionInstrument();
  play_state.current_key = song.starting_key;
  play_state.current_velocity = song.starting_velocity;
  play_state.current_tempo = song.starting_tempo;
  play_state.current_time = current_time;
}

static inline auto get_play_state_at_chord(const Song &song,
                                           const int chord_number) {
  PlayState play_state;
  initialize_playstate(song, play_state, 0);
  const auto &chords = song.chords;
  for (auto previous_chord_number = 0; previous_chord_number < chord_number;
       previous_chord_number++) {
    const auto &chord = chords.at(previous_chord_number);
    modulate(play_state, chord);
    move_time(play_state, chord);
  }
  modulate(play_state, chords.at(chord_number));
  return play_state;
}

static inline void add_frequency_to_stream(QTextStream &stream,
                                           const double frequency) {
  const auto midi_float = frequency_to_midi_number(frequency);
  const auto closest_midi = to_int(midi_float);
  const auto [octave, degree] = get_octave_degree(closest_midi - C_0_MIDI);
  const auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  static const QMap<int, QString> degrees_to_name{
      {0, QObject::tr("C")},  {1, QObject::tr("C♯")},  {2, QObject::tr("D")},
      {3, QObject::tr("E♭")}, {4, QObject::tr("E")},   {5, QObject::tr("F")},
      {6, QObject::tr("F♯")}, {7, QObject::tr("G")},   {8, QObject::tr("A♭")},
      {9, QObject::tr("A")},  {10, QObject::tr("B♭")}, {11, QObject::tr("B")},
  };

  stream << frequency << QObject::tr(" Hz ≈ ")
         << degrees_to_name[to_int(degree)] << octave;
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ") << abs(cents)
           << QObject::tr(" cents");
  }
  stream << QObject::tr("; ");
}

static inline void add_timing_to_stream(QTextStream &stream,
                                        const PlayState &play_state,
                                        const double velocity,
                                        const double beats_double) {
  stream << QObject::tr("Velocity ") << to_int(velocity) << QObject::tr("; ")
         << to_int(play_state.current_tempo) << QObject::tr(" bpm; Start at ")
         << to_int(play_state.current_time) << QObject::tr(" ms; Duration ")
         << to_int(get_duration_in_milliseconds(play_state.current_tempo,
                                                beats_double))
         << QObject::tr(" ms");
}
