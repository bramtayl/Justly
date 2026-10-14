#pragma once

#include <QtWidgets/QMessageBox>

#include "rows/Note.hpp"
#include "song/Song.hpp"
#include "sound/Player.hpp"

void initialize_play(Player& player, const Song& song);

// the pitch wheel's center, for notes that play their key's own pitch
static const auto ZERO_PITCH_BEND = 8192;

struct MidiPitch {
  short midi_number;
  int pitch_bend = ZERO_PITCH_BEND;
};

// joins a channel already set to program and pitch.pitch_bend if one has
// pitch.midi_number free (see ChannelState), so octaves and repeated pitches
// can share. Otherwise claims the least-recently-busy channel and sends it
// program and pitch bend; nullopt (after warning) if every channel is still
// sounding, so the caller should abort
[[nodiscard]] auto get_channel_number(Player& player, const Program& program,
                                      const MidiPitch& pitch,
                                      double current_time)
    -> std::optional<int>;

// the nearest MIDI key to note's exact pitch, and the bend to reach it;
// nullopt (after warning) if the frequency is out of MIDI range, so the caller
// should abort rather than play a bogus note
[[nodiscard]] auto get_closest_midi(Player& player, const PitchedNote& note,
                                    int chord_number, int note_number)
    -> std::optional<MidiPitch>;

void play_note(Player& player, int channel_number, const Program& program,
               short midi_number, short velocity, double current_time,
               double end_time);

// plays program from the current time until end_time, shared by notes and
// voice previews; get_pitch() picks the key and bend, and add_location(stream)
// says which row a too-loud warning is about. Warns and returns false if the
// key is out of range, the velocity is too loud, or no channel is free
template <typename GetPitch, typename AddLocation>
[[nodiscard]] auto play_checked_note(Player& player, const Program& program,
                                     GetPitch get_pitch, const double velocity,
                                     const double end_time,
                                     AddLocation add_location) -> bool {
  auto& parent = player.parent;
  const auto current_time = player.play_state.current_time;

  const std::optional<MidiPitch> maybe_pitch = get_pitch();
  if (!maybe_pitch.has_value()) {
    return false;
  }
  const auto& pitch = *maybe_pitch;

  const auto rounded_velocity = static_cast<short>(std::round(velocity));
  if (rounded_velocity > MAX_VELOCITY) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Velocity ") << rounded_velocity
           << QObject::tr(" exceeds ") << MAX_VELOCITY;
    add_location(stream);
    QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
    return false;
  }

  // claiming a channel sends events, so only once the note is known to play
  const auto maybe_channel_number =
      get_channel_number(player, program, pitch, current_time);
  if (!maybe_channel_number.has_value()) {
    return false;
  }

  play_note(player, *maybe_channel_number, program, pitch.midi_number,
            rounded_velocity, current_time, end_time);
  return true;
}

template <VoiceInterface SubVoice>
[[nodiscard]] auto play_voices(Player& player, const QList<SubVoice>& voices,
                               const int first_voice_number,
                               const int number_of_voices) -> bool {
  static const auto VOICE_PREVIEW_MILLISECONDS = 1000;

  const auto& play_state = player.play_state;
  const auto& programs = get_some_programs(SubVoice::is_pitched());

  for (auto voice_number = first_voice_number;
       voice_number < first_voice_number + number_of_voices; ++voice_number) {
    const auto& voice = voices.at(voice_number);
    const auto velocity =
        play_state.current_velocity * rational_to_double(voice.velocity_ratio);
    if (!play_checked_note(
            player, get_voice_program(programs, voice),
            [&voice]() -> std::optional<MidiPitch> {
              return MidiPitch{voice.get_preview_midi_number()};
            },
            velocity, play_state.current_time + VOICE_PREVIEW_MILLISECONDS,
            [&voice](QTextStream& stream) -> void {
              stream << QObject::tr(" for ")
                     << QObject::tr(SubVoice::get_pitched())
                     << QObject::tr(" voice \"") << voice.name
                     << QObject::tr("\"");
            })) {
      return false;
    }
  }
  return true;
}

template <NoteInterface SubNote>
[[nodiscard]] auto play_notes(Player& player, const Song& song,
                              const int chord_number,
                              const QList<SubNote>& sub_notes,
                              const int first_note_number,
                              const int number_of_notes) -> bool {
  const auto& play_state = player.play_state;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes; ++note_number) {
    const auto& sub_note = sub_notes.at(note_number);
    if (!play_checked_note(
            player, get_note_program(song, sub_note),
            [&]() -> std::optional<MidiPitch> {
              if constexpr (std::same_as<SubNote, PitchedNote>) {
                return get_closest_midi(player, sub_note, chord_number,
                                        note_number);
              } else {
                return MidiPitch{static_cast<short>(
                    get_note_voice(song, sub_note).midi_number)};
              }
            },
            get_note_velocity(song, sub_note, play_state.current_velocity),
            play_state.current_time + get_duration_in_milliseconds(
                                          play_state.current_tempo,
                                          rational_to_double(sub_note.beats)),
            [chord_number, note_number](QTextStream& stream) -> void {
              add_note_location<SubNote>(stream, chord_number, note_number);
            })) {
      return false;
    }
  }
  return true;
}

template <NoteInterface SubNote>
[[nodiscard]] auto play_all_notes(Player& player, const Song& song,
                                  const int chord_number,
                                  const QList<SubNote>& sub_notes) -> bool {
  return play_notes(player, song, chord_number, sub_notes, 0,
                    static_cast<int>(sub_notes.size()));
}

void update_final_time(Player& player, double new_final_time);

void play_chords(Player& player, const Song& song, int first_chord_number,
                 int number_of_chords, int wait_frames = 0);

void export_to_file(Player& player, const Song& song,
                    const QString& output_file);
