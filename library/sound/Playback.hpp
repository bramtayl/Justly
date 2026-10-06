#pragma once

#include <QtWidgets/QMessageBox>

#include "other/Song.hpp"
#include "rows/Note.hpp"
#include "sound/Player.hpp"

void initialize_play(Player& player, const Song& song);

// pitched notes always pick from the shared least-recently-free pool, since
// each one may need its own pitch bend and must wait out the previous
// occupant's release before reusing its channel. Percussion programs instead
// get a single channel permanently reserved on first use (see
// Player::percussion_channels) -- nullopt means every channel is claimed and
// the caller should warn and abort, matching channel_is_free's contract
[[nodiscard]] auto get_channel_number(Player& player, const Program& program,
                                      double current_time)
    -> std::optional<int>;

// bends channel_number to note's exact pitch, and returns the nearest MIDI
// key to play; nullopt (after warning) if the frequency is out of MIDI range,
// so the caller should abort rather than play a bogus note
[[nodiscard]] auto get_closest_midi(Player& player, const PitchedNote& note,
                                    int channel_number, int chord_number,
                                    int note_number) -> std::optional<short>;

void play_note(Player& player, int channel_number, const Program& program,
               short midi_number, short velocity, double current_time,
               double end_time);

// plays program from the current time until end_time, shared by notes and
// voice previews; get_midi(channel_number) picks the key (a pitched note
// bends that channel to its exact pitch first), and add_location(stream)
// says which row a too-loud warning is about. Warns and returns false if no
// channel is free, the key is out of range, or the velocity is too loud
template <typename GetMidi, typename AddLocation>
[[nodiscard]] auto play_checked_note(Player& player, const Program& program,
                                     GetMidi get_midi, const double velocity,
                                     const double end_time,
                                     AddLocation add_location) -> bool {
  auto& parent = player.parent;
  const auto current_time = player.play_state.current_time;

  const auto maybe_channel_number =
      get_channel_number(player, program, current_time);
  if (!maybe_channel_number.has_value()) {
    return false;
  }
  const auto channel_number = *maybe_channel_number;

  const std::optional<short> maybe_midi_number = get_midi(channel_number);
  if (!maybe_midi_number.has_value()) {
    return false;
  }

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

  play_note(player, channel_number, program, *maybe_midi_number,
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
            [&voice](int /*channel_number*/) -> std::optional<short> {
              return voice.get_preview_midi_number();
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
            [&](const int channel_number) -> std::optional<short> {
              if constexpr (std::same_as<SubNote, PitchedNote>) {
                return get_closest_midi(player, sub_note, channel_number,
                                        chord_number, note_number);
              } else {
                return static_cast<short>(
                    get_note_voice(song, sub_note).midi_number);
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
