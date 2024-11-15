#pragma once

#include <QList>
#include <QString>
#include <fluidsynth.h>

struct Instrument;
struct PercussionSet;
struct PercussionInstrument;
class QWidget;
struct Song;

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

void send_event_at(const Player &player, double time);

void player_play_chords(Player &player, const Song &song,
                        int first_chord_number, int number_of_chords);
void player_play_pitched_notes(Player &player, const Song &song,
                               int chord_number, int first_note_number,
                               int number_of_notes);
void player_play_unpitched_notes(Player &player, const Song &song,
                                 int chord_number, int first_note_number,
                                 int number_of_notes);

void stop_playing(const Player &player);
void export_song_to_file(Player &player, const Song &song,
                         const QString &output_file);
