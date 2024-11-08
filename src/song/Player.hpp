#pragma once

#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <cmath>
#include <concepts>
#include <fluidsynth.h>

#include "row/note/Note.hpp"

struct Instrument;
struct PercussionSet;
struct PercussionInstrument;
struct Song;
class QWidget;

struct Chord;

const auto NUMBER_OF_MIDI_CHANNELS = 16;
const auto MILLISECONDS_PER_SECOND = 1000;
static const auto HALFSTEPS_PER_OCTAVE = 12;
static const auto MAX_VELOCITY = 127;

[[nodiscard]] auto get_midi(double key) -> double;

struct Player {
  // data
  QWidget &parent;

  // play state fields
  QList<double> channel_schedules;

  const Instrument *current_instrument_pointer;
  const PercussionSet *current_percussion_set_pointer;
  const PercussionInstrument *current_percussion_instrument_pointer;

  double starting_time = 0;
  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  fluid_settings_t *const settings_pointer;
  fluid_event_t *const event_pointer;
  fluid_sequencer_t *const sequencer_pointer;
  fluid_synth_t *synth_pointer;
  const unsigned int soundfont_id;
  const fluid_seq_id_t sequencer_id;

  fluid_audio_driver_t *audio_driver_pointer = nullptr;

  // methods
  explicit Player(QWidget &parent);
  ~Player();

  // prevent moving and copying
  Player(const Player &) = delete;
  auto operator=(const Player &) -> Player = delete;
  Player(Player &&) = delete;
  auto operator=(Player &&) -> Player = delete;
};

void initialize_play(Player &player, const Song &song);
void modulate(Player &player, const Chord &chord);
void modulate_before_chord(Player &player, const Song &song,
                           int next_chord_number);
void send_event_at(const Player &player, double time);

auto get_beat_time(double tempo) -> double;

void update_final_time(Player &player, double new_final_time);

template <std::derived_from<Note> SubNote>
void play_notes(Player &player, int chord_number,
                const QList<SubNote> &sub_notes, int first_note_number,
                int number_of_notes) {
  auto &parent = player.parent;
  auto *event_pointer = player.event_pointer;
  auto &channel_schedules = player.channel_schedules;
  const auto soundfont_id = player.soundfont_id;

  const auto current_time = player.current_time;
  const auto current_velocity = player.current_velocity;
  const auto current_tempo = player.current_tempo;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    auto channel_number = -1;
    for (auto channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      if (current_time >= channel_schedules.at(channel_index)) {
        channel_number = channel_index;
      }
    }
    if (channel_number == -1) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Out of MIDI channels");
      add_note_location(stream, chord_number, note_number,
                        SubNote::get_note_type());
      stream << QObject::tr(". Not playing note.");
      QMessageBox::warning(&parent, QObject::tr("MIDI channel error"), message);
      return;
    }
    const auto &sub_note = sub_notes.at(note_number);

    const auto &program =
        sub_note.get_program(player, chord_number, note_number);

    fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                               program.bank_number, program.preset_number);
    send_event_at(player, current_time);

    auto midi_number = sub_note.get_closest_midi(player, channel_number,
                                                 chord_number, note_number);

    auto velocity = current_velocity * sub_note.velocity_ratio.to_double();
    short new_velocity = 1;
    if (velocity > MAX_VELOCITY) {
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
             << MAX_VELOCITY;
      add_note_location(stream, chord_number, note_number,
                        SubNote::get_note_type());
      stream << QObject::tr(". Playing with velocity ") << MAX_VELOCITY
             << QObject::tr(".");
      QMessageBox::warning(&parent, QObject::tr("Velocity error"), message);
    } else {
      new_velocity = static_cast<short>(std::round(velocity));
    }
    fluid_event_noteon(event_pointer, channel_number, midi_number,
                       new_velocity);
    send_event_at(player, current_time);

    auto end_time = current_time + get_beat_time(current_tempo) *
                                       sub_note.beats.to_double() *
                                       MILLISECONDS_PER_SECOND;

    fluid_event_noteoff(event_pointer, channel_number, midi_number);
    send_event_at(player, end_time);

    channel_schedules[channel_number] = end_time;
  }
}

void play_chords(Player &player, const Song &song, int first_chord_number,
                 int number_of_chords, int wait_frames = 0);
void stop_playing(const Player &player);
void export_song_to_file(Player &player, const Song &song,
                         const QString &output_file);
