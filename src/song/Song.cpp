#include <QObject>
#include <QTextStream>
#include <QtGlobal>
#include <cstdlib>

#include "abstract_rational/interval/Interval.hpp"
#include "other/other.hpp"
#include "row/chord/Chord.hpp"
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

static auto get_note_text(int degree) -> const char * {
  switch (degree) {
  case c_degree:
    return "C";
  case c_sharp_degree:
    return "C♯";
  case d_degree:
    return "D";
  case e_flat_degree:
    return "E♭";
  case e_degree:
    return "E";
  case f_degree:
    return "F";
  case f_sharp_degree:
    return "F♯";
  case g_degree:
    return "G";
  case a_flat_degree:
    return "A♭";
  case a_degree:
    return "A";
  case b_flat_degree:
    return "B♭";
  case b_degree:
    return "B";
  default:
    Q_ASSERT(false);
    return "";
  }
}

auto get_key_text(const Song &song, int chord_number, double ratio) -> QString {
  const auto &chords = song.chords;
  auto key = song.starting_key;
  for (auto previous_chord_number = 0; previous_chord_number <= chord_number;
       previous_chord_number++) {
    key = key * chords.at(previous_chord_number).interval.to_double();
  }
  key = key * ratio;
  auto midi_float = get_midi(key);
  auto closest_midi = to_int(midi_float);
  auto difference_from_c = closest_midi - C_0_MIDI;
  auto octave =
      difference_from_c / HALFSTEPS_PER_OCTAVE; // floor integer division
  auto degree = difference_from_c - octave * HALFSTEPS_PER_OCTAVE;
  auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  QString result;
  QTextStream stream(&result);
  stream << key << QObject::tr(" Hz; ") << QObject::tr(get_note_text(degree))
         << octave;
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ")
           << abs(cents) << QObject::tr(" cents");
  }
  return result;
}
