#include "other/Song.hpp"

#include "rows/Chord.hpp"

Song::Song() : starting_key(midi_number_to_frequency(DEFAULT_STARTING_MIDI)) {}

auto get_octave_degree(int midi_interval) -> std::tuple<int, int> {
  const int octave =
      to_int(std::floor((1.0 * midi_interval) / HALFSTEPS_PER_OCTAVE));
  return std::make_tuple(octave,
                         midi_interval - (octave * HALFSTEPS_PER_OCTAVE));
}

auto initialize_playstate(const Song& song, const double current_time)
    -> PlayState {
  return {.current_time = current_time,
          .current_key = song.starting_key,
          .current_velocity = song.starting_velocity,
          .current_tempo = song.starting_tempo};
}

auto get_play_state_before_chord(const Song& song, const int chord_number)
    -> PlayState {
  auto play_state = initialize_playstate(song);
  walk_chords(play_state, song.chords, 0, chord_number,
              [](int /*chord_number*/, const Chord& /*chord*/) -> void {});
  return play_state;
}

auto get_play_state_at_chord(const Song& song, const int chord_number)
    -> PlayState {
  auto play_state = get_play_state_before_chord(song, chord_number);
  modulate(play_state, song.chords.at(chord_number));
  return play_state;
}

auto get_chord_start_times(const Song& song) -> QList<double> {
  QList<double> chord_start_times;
  auto play_state = initialize_playstate(song);
  walk_chords(play_state, song.chords, 0, static_cast<int>(song.chords.size()),
              [&chord_start_times, &play_state](
                  int /*chord_number*/, const Chord& /*chord*/) -> void {
                chord_start_times.push_back(play_state.current_time);
              });
  return chord_start_times;
}

auto get_note_name(const int closest_midi) -> QString {
  static const QMap<int, QString> degrees_to_name{
      {0, QObject::tr("C")},  {1, QObject::tr("C♯")},  {2, QObject::tr("D")},
      {3, QObject::tr("E♭")}, {4, QObject::tr("E")},   {5, QObject::tr("F")},
      {6, QObject::tr("F♯")}, {7, QObject::tr("G")},   {8, QObject::tr("A♭")},
      {9, QObject::tr("A")},  {10, QObject::tr("B♭")}, {11, QObject::tr("B")},
  };
  const auto [octave, degree] = get_octave_degree(closest_midi - C_0_MIDI);
  return degrees_to_name[degree] + QString::number(octave);
}

void add_frequency_to_stream(QTextStream& stream, const double frequency) {
  static const auto CENTS_PER_HALFSTEP = 100;
  const auto midi_float = frequency_to_midi_number(frequency);
  const auto closest_midi = to_int(midi_float);
  const auto cents = to_int((midi_float - closest_midi) * CENTS_PER_HALFSTEP);

  stream << frequency << QObject::tr(" Hz ≈ ") << get_note_name(closest_midi);
  if (cents != 0) {
    stream << QObject::tr(cents >= 0 ? " + " : " − ") << abs(cents)
           << QObject::tr(" cents");
  }
  stream << QObject::tr("; ");
}

void add_timing_to_stream(QTextStream& stream, const PlayState& play_state,
                          const double velocity, const double beats_double) {
  stream << QObject::tr("Velocity ") << to_int(velocity) << QObject::tr("; ")
         << to_int(play_state.current_tempo) << QObject::tr(" bpm; Start at ")
         << to_int(play_state.current_time) << QObject::tr(" ms; Duration ")
         << to_int(get_duration_in_milliseconds(play_state.current_tempo,
                                                beats_double))
         << QObject::tr(" ms");
}
