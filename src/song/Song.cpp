#include <QObject>
#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <cstdlib>

#include "chord/Chord.hpp"
#include "interval/Interval.hpp"
#include "song/ControlId.hpp"
#include "song/Player.hpp"
#include "song/Song.hpp"

static const auto CENTS_PER_HALFSTEP = 100;

static const auto C_0_MIDI = 12;

enum Degree {
  c_degree,
  c_sharp_degree,
  d_degree,
  e_flat_degree,
  e_degree,
  f_degree,
  f_sharp_degree,
  g_degree,
  a_flat_degree,
  a_degree,
  b_flat_degree,
  b_degree
};

auto get_double(const Song &song, ControlId command_id) -> double {
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

auto get_key_text(const Song &song, int chord_number, double ratio) -> QString {
  const auto &chords = song.chords;
  auto key = song.starting_key;
  for (auto previous_chord_number = 0; previous_chord_number <= chord_number;
       previous_chord_number++) {
    key = key * chords.at(previous_chord_number).interval.to_double();
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
  stream << key << QObject::tr(" Hz; ");

  switch (degree) {
  case c_degree:
    stream << QObject::tr("C");
    break;
  case c_sharp_degree:
    stream << QObject::tr("C♯");
    break;
  case d_degree:
    stream << QObject::tr("D");
    break;
  case e_flat_degree:
    stream << QObject::tr("E♭");
    break;
  case e_degree:
    stream << QObject::tr("E");
    break;
  case f_degree:
    stream << QObject::tr("F");
    break;
  case f_sharp_degree:
    stream << QObject::tr("F♯");
    break;
  case g_degree:
    stream << QObject::tr("G");
    break;
  case a_flat_degree:
    stream << QObject::tr("A♭");
    break;
  case a_degree:
    stream << QObject::tr("A");
    break;
  case b_flat_degree:
    stream << QObject::tr("B♭");
    break;
  case b_degree:
    stream << QObject::tr("B");
    break;
  default:
    Q_ASSERT(false);
    break;
  }
  stream << octave;
  if (cents != 0) {
    stream << " " << (cents >= 0 ? "+" : "−") << " " << abs(cents)
           << QObject::tr(" cents");
  }
  return result;
}
