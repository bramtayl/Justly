#include "song/Player.hpp"

#include <QList>
#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QtGlobal>
#include <cmath>
#include <concepts>
#include <memory>
#include <thread>

#include "abstract_rational/interval/Interval.hpp"
#include "abstract_rational/rational/Rational.hpp"
#include "named/program/Program.hpp"
#include "row/chord/Chord.hpp"
#include "song/Song.hpp"

// play settings
static const auto SECONDS_PER_MINUTE = 60;
static const auto VERBOSE_FLUIDSYNTH = false;

static const auto START_END_MILLISECONDS = 500;

static const auto CONCERT_A_FREQUENCY = 440;
static const auto CONCERT_A_MIDI = 69;

static void set_fluid_int(fluid_settings_t *settings_pointer, const char *field,
                          int value) {
  auto result = fluid_settings_setint(settings_pointer, field, value);
  Q_ASSERT(result == FLUID_OK);
}

static void set_fluid_string(fluid_settings_t *settings_pointer,
                             const char *field, const char *value) {
  auto result = fluid_settings_setstr(settings_pointer, field, value);
  Q_ASSERT(result == FLUID_OK);
}

[[nodiscard]] static auto get_settings_pointer() {
  fluid_settings_t *settings_pointer = new_fluid_settings();
  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    set_fluid_int(settings_pointer, "synth.cpu-cores", static_cast<int>(cores));
  }
#ifdef __linux__
  fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#endif
  return settings_pointer;
}

[[nodiscard]] auto get_beat_time(double tempo) -> double {
  return SECONDS_PER_MINUTE / tempo;
}

void send_event_at(const Player &player, double time) {
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
  auto duplicated = fluid_settings_dupstr(settings_pointer, "audio.driver",
                                          default_driver_pointer.get());
  Q_ASSERT(duplicated == FLUID_OK);

  delete_audio_driver(player);

#ifndef NO_REALTIME_AUDIO
  auto *new_audio_driver =
      new_fluid_audio_driver(settings_pointer, player.synth_pointer);
  if (new_audio_driver == nullptr) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot start audio driver \"")
           << *default_driver_pointer << QObject::tr("\"");
    QMessageBox::warning(&player.parent, QObject::tr("Audio driver error"),
                         message);
  } else {
    player.audio_driver_pointer = new_audio_driver;
  }
#endif
}

void update_final_time(Player &player, double new_final_time) {
  if (new_final_time > player.final_time) {
    player.final_time = new_final_time;
  }
};

auto get_midi(double key) -> double {
  return HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY) +
         CONCERT_A_MIDI;
}

Player::Player(QWidget &parent_input)
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
  player.current_tempo = song.starting_tempo;
  player.current_time = fluid_sequencer_get_tick(player.sequencer_pointer);

  auto &channel_schedules = player.channel_schedules;
  for (auto index = 0; index < NUMBER_OF_MIDI_CHANNELS; index = index + 1) {
    Q_ASSERT(index < channel_schedules.size());
    channel_schedules[index] = 0;
  }
}

void modulate(Player &player, const Chord &chord) {
  player.current_key = player.current_key * chord.interval.to_double();
  player.current_velocity =
      player.current_velocity * chord.velocity_ratio.to_double();
  player.current_tempo = player.current_tempo * chord.tempo_ratio.to_double();
  const auto *chord_instrument_pointer = chord.instrument_pointer;
  if (chord_instrument_pointer != nullptr) {
    player.current_instrument_pointer = chord_instrument_pointer;
  }

  const auto *chord_percussion_set_pointer = chord.percussion_set_pointer;
  if (chord_percussion_set_pointer != nullptr) {
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

template <std::derived_from<Note> SubNote>
static void play_all_notes(Player &player, int chord_number,
                           const QList<SubNote> &sub_notes,
                           int first_note_number) {
  play_notes(player, chord_number, sub_notes, first_note_number,
             static_cast<int>(sub_notes.size()));
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
    play_all_notes(player, chord_number, chord.pitched_notes, 0);
    play_all_notes(player, chord_number, chord.unpitched_notes, 0);
    auto new_current_time =
        player.current_time + get_beat_time(player.current_tempo) *
                                  chord.beats.to_double() *
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
  set_fluid_string(settings_pointer, "audio.file.name",
                   output_file.toStdString().c_str());

  set_fluid_int(settings_pointer, "synth.lock-memory", 0);

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
  set_fluid_int(settings_pointer, "synth.lock-memory", 1);
  start_real_time(player);
}
