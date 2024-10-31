#include "song/Player.hpp"

#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <memory>
#include <thread>

#include "chord/Chord.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "named/Named.hpp"
#include "other/AbstractInstrument.hpp"
#include "other/other.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "pitched_note/PitchedNote.hpp"
#include "rational/Rational.hpp"
#include "song/Song.hpp"
#include "unpitched_note/UnpitchedNote.hpp"

// play settings
static const auto SECONDS_PER_MINUTE = 60;
static const auto MILLISECONDS_PER_SECOND = 1000;
static const auto VERBOSE_FLUIDSYNTH = false;
static const auto NUMBER_OF_MIDI_CHANNELS = 16;
static const auto BEND_PER_HALFSTEP = 4096;
static const auto ZERO_BEND_HALFSTEPS = 2;
static const auto START_END_MILLISECONDS = 500;

static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;

[[nodiscard]] static auto get_settings_pointer() {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    auto cores_result = fluid_settings_setint(
        settings_pointer, "synth.cpu-cores", static_cast<int>(cores));
    Q_ASSERT(cores_result == FLUID_OK);
  }
  if (VERBOSE_FLUIDSYNTH) {
    auto verbose_result =
        fluid_settings_setint(settings_pointer, "synth.verbose", 1);
    Q_ASSERT(verbose_result == FLUID_OK);
  }
  return settings_pointer;
}

[[nodiscard]] static auto get_beat_time(double tempo) {
  return SECONDS_PER_MINUTE / tempo;
}

static void send_event_at(const Player &player, double time) {
  Q_ASSERT(time >= 0);
  auto result =
      fluid_sequencer_send_at(player.sequencer_pointer, player.event_pointer,
                              static_cast<unsigned int>(std::round(time)), 1);
  Q_ASSERT(result == FLUID_OK);
};

static void delete_audio_driver(Player &player) {
  auto *audio_driver_pointer = player.audio_driver_pointer;
  if (audio_driver_pointer != nullptr) {
    delete_fluid_audio_driver(audio_driver_pointer);
    player.audio_driver_pointer = nullptr;
  }
}

static void start_real_time(Player &player) {
  auto *settings_pointer = player.settings_pointer;

  auto default_driver_pointer = std::make_unique<char *>();
  fluid_settings_dupstr(settings_pointer, "audio.driver",
                        default_driver_pointer.get());
  Q_ASSERT(default_driver_pointer != nullptr);

  delete_audio_driver(player);

  QString default_driver(*default_driver_pointer);
#ifdef __linux__
  default_driver = "pulseaudio";
#endif
  fluid_settings_setstr(settings_pointer, "audio.driver",
                        default_driver.toStdString().c_str());

#ifndef NO_REALTIME_AUDIO
  auto *new_audio_driver =
      new_fluid_audio_driver(settings_pointer, player.synth_pointer);
  if (new_audio_driver == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot start audio driver \"") << default_driver
           << QObject::tr("\"");
    QMessageBox::warning(&player.parent,
                         QObject::tr("Audio driver error"), message);
  } else {
    player.audio_driver_pointer = new_audio_driver;
  }
#endif
}

[[nodiscard]] static auto get_open_channel_number(const Player &player,
                                                  int chord_number,
                                                  int note_number,
                                                  const char *note_type) {
  const auto &channel_schedules = player.channel_schedules;
  const auto current_time = player.current_time;

  for (auto channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
       channel_index = channel_index + 1) {
    if (current_time >= channel_schedules.at(channel_index)) {
      return channel_index;
    }
  }
  QString message;
  QTextStream stream(&message);
  stream << QObject::tr("Out of MIDI channels");
  add_note_location(stream, chord_number, note_number, note_type);
  stream << QObject::tr(". Not playing note.");
  QMessageBox::warning(&player.parent, QObject::tr("MIDI channel error"),
                       message);
  return -1;
}

static void change_instrument(const Player &player, int channel_number,
                              const AbstractInstrument &abstract_instrument) {
  fluid_event_program_select(
      player.event_pointer, channel_number, player.soundfont_id,
      abstract_instrument.bank_number, abstract_instrument.preset_number);
  send_event_at(player, player.current_time);
}

static void update_final_time(Player &player, double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
};

static void play_note(Player &player, int channel_number, short midi_number,
                      const Rational &beats, const Rational &velocity_ratio,
                      int time_offset, int chord_number, int item_number,
                      const QString &item_description) {
  const auto current_time = player.current_time;
  auto *event_pointer = player.event_pointer;

  auto velocity = player.current_velocity * rational_to_double(velocity_ratio);
  short new_velocity = 1;
  if (velocity > MAX_VELOCITY) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Velocity ") << velocity << QObject::tr(" exceeds ")
           << MAX_VELOCITY << QObject::tr(" for chord ") << chord_number + 1
           << QObject::tr(", ") << item_description << " " << item_number + 1
           << QObject::tr(". Playing with velocity ") << MAX_VELOCITY
           << QObject::tr(".");
    QMessageBox::warning(&player.parent, QObject::tr("Velocity error"),
                         message);
  } else {
    new_velocity = static_cast<short>(std::round(velocity));
  }
  fluid_event_noteon(event_pointer, channel_number, midi_number, new_velocity);
  send_event_at(player, current_time + time_offset);

  auto end_time = current_time + get_beat_time(player.current_tempo) *
                                     rational_to_double(beats) *
                                     MILLISECONDS_PER_SECOND;

  fluid_event_noteoff(event_pointer, channel_number, midi_number);
  send_event_at(player, end_time);
  Q_ASSERT(channel_number < player.channel_schedules.size());
  player.channel_schedules[channel_number] = end_time;
  update_final_time(player, end_time);
}

auto get_midi(double key) -> double {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

Player::Player(QWidget& parent_input)
    : parent(parent_input),
      channel_schedules(QList<double>(NUMBER_OF_MIDI_CHANNELS, 0)),
      current_instrument_pointer(nullptr),
      current_percussion_set_pointer(nullptr),
      current_percussion_instrument_pointer(nullptr),
      settings_pointer(get_settings_pointer()),
      event_pointer(new_fluid_event()),
      sequencer_pointer(new_fluid_sequencer2(0)),
      synth_pointer(new_fluid_synth(settings_pointer)),
      soundfont_id(get_soundfont_id(synth_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  fluid_event_set_dest(event_pointer, sequencer_id);
  start_real_time(*this);
}

Player::~Player() {
  delete_audio_driver(*this);
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
}

void initialize_play(Player &player, const Song &song) {
  player.current_instrument_pointer = nullptr;
  player.current_percussion_set_pointer = nullptr;
  player.current_percussion_instrument_pointer = nullptr;
  player.current_key = song.starting_key;
  player.current_velocity = song.starting_velocity;
  player.current_tempo = song.gain;
  player.current_time = fluid_sequencer_get_tick(player.sequencer_pointer);

  auto &channel_schedules = player.channel_schedules;
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

void modulate(Player &player, const Chord &chord) {
  player.current_key = player.current_key * interval_to_double(chord.interval);
  player.current_velocity =
      player.current_velocity * rational_to_double(chord.velocity_ratio);
  player.current_tempo =
      player.current_tempo * rational_to_double(chord.tempo_ratio);
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    player.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto *chord_percussion_set_pointer = chord.percussion_set_pointer;
  if (chord_percussion_set_pointer == nullptr) {
    player.current_percussion_set_pointer = chord_percussion_set_pointer;
  }

  const auto *chord_percussion_instrument_pointer =
      chord.percussion_instrument_pointer;
  if (chord_percussion_instrument_pointer != nullptr) {
    player.current_percussion_instrument_pointer =
        chord_percussion_instrument_pointer;
  }
}

void modulate_before_chord(Player &player, const Song &song,
                           int next_chord_number) {
  const auto &chords = song.chords;
  if (next_chord_number > 0) {
    for (auto chord_number = 0; chord_number < next_chord_number;
         chord_number = chord_number + 1) {
      modulate(player, chords.at(chord_number));
    }
  }
}

void play_pitched_notes(Player &player, int chord_number, const Chord &chord,
                        int first_note_number,
                        int number_of_notes) {
  auto& parent = player.parent;
  auto *event_pointer = player.event_pointer;
  const auto *current_instrument_pointer = player.current_instrument_pointer;
  const auto current_key = player.current_key;
  const auto current_time = player.current_time;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    auto channel_number =
        get_open_channel_number(player, chord_number, note_number, "pitched");
    if (channel_number != -1) {
      const auto &pitched_note = chord.pitched_notes.at(note_number);

      change_instrument(
          player, channel_number,
          substitute_named_for(parent,
                               pitched_note.instrument_pointer,
                               current_instrument_pointer, chord_number,
                               note_number, "pitched", "instrument", "Marimba",
                               "Instrument error"));

      auto midi_float =
          get_midi(current_key * interval_to_double(pitched_note.interval));
      auto closest_midi = static_cast<short>(round(midi_float));

      fluid_event_pitch_bend(event_pointer, channel_number,
                             static_cast<int>(round((midi_float - closest_midi +
                                                     ZERO_BEND_HALFSTEPS) *
                                                    BEND_PER_HALFSTEP)));
      send_event_at(player, current_time + 1);

      play_note(player, channel_number, closest_midi, pitched_note.beats,
                pitched_note.velocity_ratio, 2, chord_number, note_number,
                QObject::tr("pitched note"));
    }
  }
}

void play_unpitched_notes(Player &player, int chord_number, const Chord &chord,
                          int first_note_number,
                          int number_of_notes) {
  auto& parent = player.parent;
  const auto *current_percussion_instrument_pointer =
      player.current_percussion_instrument_pointer;
  const auto *current_percussion_set_pointer =
      player.current_percussion_set_pointer;

  for (auto note_number = first_note_number;
       note_number < first_note_number + number_of_notes;
       note_number = note_number + 1) {
    auto channel_number =
        get_open_channel_number(player, chord_number, note_number, "unpitched");
    if (channel_number != -1) {
      const auto &unpitched_note = chord.unpitched_notes.at(note_number);

      change_instrument(
          player, channel_number,
          substitute_named_for(parent,
                               unpitched_note.percussion_set_pointer,
                               current_percussion_set_pointer, chord_number,
                               note_number, "unpitched", "percussion set",
                               "Standard", "Percussion set error"));

      play_note(player, channel_number,
                substitute_named_for(
                    parent,
                    unpitched_note.percussion_instrument_pointer,
                    current_percussion_instrument_pointer, chord_number,
                    note_number, "unpitched", "percussion instrument",
                    "Tambourine", "Percussion instrument error")
                    .midi_number,
                unpitched_note.beats, unpitched_note.velocity_ratio, 1,
                chord_number, note_number, QObject::tr("unpitched note"));
    }
  }
}

void play_chords(Player &player, const Song &song, int first_chord_number,
                 int number_of_chords, int wait_frames) {
  auto start_time = player.current_time + wait_frames;
  player.current_time = start_time;
  update_final_time(player, start_time);
  const auto &chords = song.chords;
  for (auto chord_number = first_chord_number;
       chord_number < first_chord_number + number_of_chords;
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);

    modulate(player, chord);
    play_pitched_notes(player, chord_number, chord, 0,
                       static_cast<int>(chord.pitched_notes.size()));
    play_unpitched_notes(player, chord_number, chord, 0,
                         static_cast<int>(chord.unpitched_notes.size()));
    auto new_current_time =
        player.current_time + (get_beat_time(player.current_tempo) *
                               rational_to_double(chord.beats)) *
                                  MILLISECONDS_PER_SECOND;
    player.current_time = new_current_time;
    update_final_time(player, new_current_time);
  }
}

void stop_playing(const Player &player) {
  auto *event_pointer = player.event_pointer;
  auto *sequencer_pointer = player.sequencer_pointer;

  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    fluid_event_all_sounds_off(event_pointer, channel_number);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

void export_song_to_file(Player &player, const Song &song,
                         const QString &output_file) {
  auto *settings_pointer = player.settings_pointer;
  auto *event_pointer = player.event_pointer;

  stop_playing(player);

  delete_audio_driver(player);
  auto file_result = fluid_settings_setstr(settings_pointer, "audio.file.name",
                                           output_file.toStdString().c_str());
  Q_ASSERT(file_result == FLUID_OK);

  auto unlock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);
  Q_ASSERT(unlock_result == FLUID_OK);

  auto finished = false;
  auto finished_timer_id = fluid_sequencer_register_client(
      player.sequencer_pointer, "finished timer",
      [](unsigned int /*time*/, fluid_event_t * /*event*/,
         fluid_sequencer_t * /*seq*/, void *data_pointer) {
        auto *finished_pointer = static_cast<bool *>(data_pointer);
        Q_ASSERT(finished_pointer != nullptr);
        *finished_pointer = true;
      },
      &finished);
  Q_ASSERT(finished_timer_id >= 0);

  initialize_play(player, song);
  play_chords(player, song, 0, static_cast<int>(song.chords.size()),
              START_END_MILLISECONDS);

  fluid_event_set_dest(event_pointer, finished_timer_id);
  fluid_event_timer(event_pointer, nullptr);
  send_event_at(player, player.final_time + START_END_MILLISECONDS);

  auto *renderer_pointer = new_fluid_file_renderer(player.synth_pointer);
  Q_ASSERT(renderer_pointer != nullptr);
  while (!finished) {
    auto process_result = fluid_file_renderer_process_block(renderer_pointer);
    Q_ASSERT(process_result == FLUID_OK);
  }
  delete_fluid_file_renderer(renderer_pointer);

  fluid_event_set_dest(event_pointer, player.sequencer_id);
  auto lock_result =
      fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  Q_ASSERT(lock_result == FLUID_OK);
  start_real_time(player);
}
