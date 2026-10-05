#pragma once

#include "rows/Note.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "sound/PlayState.hpp"

struct Chord;
struct PitchedNote;

static const auto C_0_MIDI = 12;
static const auto DEFAULT_STARTING_MIDI = MIDDLE_C_MIDI;
static const auto DEFAULT_STARTING_TEMPO = 100;
static const auto DEFAULT_STARTING_VELOCITY = 64;

struct Song {
  double starting_key;
  double starting_velocity = DEFAULT_STARTING_VELOCITY;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QList<Chord> chords;
  QList<PitchedVoice> pitched_voices;
  QList<UnpitchedVoice> unpitched_voices;

  Song();
};

// the voices that a SubNote's voice_name refers to
template <NoteInterface SubNote>
[[nodiscard]] static auto get_voices(const Song& song) -> const auto& {
  if constexpr (std::same_as<SubNote, PitchedNote>) {
    return song.pitched_voices;
  } else {
    return song.unpitched_voices;
  }
}

// the voice a note's voice_name refers to; see get_voice
template <NoteInterface SubNote>
[[nodiscard]] static auto get_note_voice(const Song& song, const SubNote& note)
    -> const auto& {
  return get_voice(get_voices<SubNote>(song), note.voice_name);
}

template <NoteInterface SubNote>
[[nodiscard]] static auto get_note_program(const Song& song,
                                           const SubNote& note)
    -> const Program& {
  const auto& voice = get_note_voice(song, note);
  return get_voice_program(
      get_some_programs(std::remove_cvref_t<decltype(voice)>::is_pitched()),
      voice);
}

// scales the chord's velocity by the note's ratio and its voice's ratio
template <NoteInterface SubNote>
[[nodiscard]] static auto get_note_velocity(const Song& song,
                                            const SubNote& note,
                                            const double current_velocity)
    -> double {
  return current_velocity * rational_to_double(note.velocity_ratio) *
         rational_to_double(get_note_voice(song, note).velocity_ratio);
}

[[nodiscard]] auto get_octave_degree(int midi_interval) -> std::tuple<int, int>;

[[nodiscard]] auto initialize_playstate(const Song& song,
                                        double current_time = 0) -> PlayState;

// the play state once every chord before chord_number has played, i.e. at
// chord_number's start time, but before chord_number modulates it
[[nodiscard]] auto get_play_state_before_chord(const Song& song,
                                               int chord_number) -> PlayState;

[[nodiscard]] auto get_play_state_at_chord(const Song& song, int chord_number)
    -> PlayState;

// each chord's start time, in chord order -- chords are laid out back-to-
// back with no gaps, so a chord's own end time is simply the next chord's
// start
[[nodiscard]] auto get_chord_start_times(const Song& song) -> QList<double>;

[[nodiscard]] auto get_note_name(int closest_midi) -> QString;

void add_frequency_to_stream(QTextStream& stream, double frequency);

void add_timing_to_stream(QTextStream& stream, const PlayState& play_state,
                          double velocity, double beats_double);
