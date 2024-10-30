#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <cstdlib>

#include "chord/Chord.hpp"
#include "interval/Interval.hpp"
#include "song/ControlId.hpp"
#include "song/Song.hpp"

static const auto CENTS_PER_HALFSTEP = 100;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;

static const auto C_0_MIDI = 12;

enum Degree {
  c_degree = 0,
  c_sharp_degree = 1,
  d_degree = 2,
  e_flat_degree = 3,
  e_degree = 4,
  f_degree = 5,
  f_sharp_degree = 6,
  g_degree = 7,
  a_flat_degree = 8,
  a_degree = 9,
  b_flat_degree = 10,
  b_degree = 11
};

auto get_double(const Song &song,
                       ControlId command_id) -> double {
  switch (command_id) {
  case gain_id:
    return song.gain;
  case starting_key_id:
    return song.starting_key;
  case starting_velocity_id:
    return song.starting_velocity;
  case starting_tempo_id:
    return song.starting_tempo;
  }
};

auto get_midi(double key) -> double {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

auto get_key_text(const Song &song, int chord_number,
                  double ratio) -> QString {
  const auto& chords = song.chords;
  auto key = song.starting_key;
  for (auto previous_chord_number = 0; previous_chord_number <= chord_number;
       previous_chord_number++) {
    key = key *
          interval_to_double(chords.at(previous_chord_number).interval);
  }
  key = key * ratio;
  auto midi_float = get_midi(key);
  auto closest_midi = static_cast<int>(round(midi_float));
  auto difference_from_c = closest_midi - C_0_MIDI;
  auto octave =
      difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  auto degree = difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
  auto cents =
      static_cast<int>(round((midi_float - closest_midi) * CENTS_PER_HALFSTEP));

  QString result;
  QTextStream stream(&result);
  stream << key << " Hz; ";

  Q_ASSERT(degree >= 0);
  Q_ASSERT(degree <= 11);
  switch (static_cast<Degree>(degree)) {
  case c_degree:
    stream << "C";
    break;
  case c_sharp_degree:
    stream << "C♯";
    break;
  case d_degree:
    stream << "D";
    break;
  case e_flat_degree:
    stream << "E♭";
    break;
  case e_degree:
    stream << "E";
    break;
  case f_degree:
    stream << "F";
    break;
  case f_sharp_degree:
    stream << "F♯";
    break;
  case g_degree:
    stream << "G";
    break;
  case a_flat_degree:
    stream << "A♭";
    break;
  case a_degree:
    stream << "A";
    break;
  case b_flat_degree:
    stream << "B♭";
    break;
  case b_degree:
    stream << "B";
    break;
  }
  stream << octave;
  if (cents != 0) {
    stream << " " << (cents >= 0 ? "+" : "−") << " " << abs(cents) << " cents";
  }
  return result;
}
