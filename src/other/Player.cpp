#include "justly/Player.hpp"

#include <qassert.h>           // for Q_ASSERT
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qlogging.h>          // for qWarning
#include <qmessagebox.h>       // for QMessageBox
#include <qobject.h>           // for QObject
#include <qstring.h>           // for QString
#include <qthread.h>           // for QThread

#include <cmath>       // for log2, round
#include <cstdint>     // for int16_t, uint64_t
#include <filesystem>  // for exists
#include <sstream>     // for operator<<, basic_ostream
#include <thread>      // for thread

#include "justly/Chord.hpp"             // for Chord
#include "justly/Instrument.hpp"        // for Instrument, get_instrument_po...
#include "justly/Interval.hpp"          // for Interval
#include "justly/Note.hpp"              // for Note
#include "justly/Rational.hpp"          // for Rational
#include "justly/Song.hpp"              // for Song
#include "other/private_constants.hpp"  // for PERCENT

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto MAX_VELOCITY = 127;
const unsigned int MILLISECONDS_PER_SECOND = 1000;
const auto BEND_PER_HALFSTEP = 4096;
const auto ZERO_BEND_HALFSTEPS = 2;
// insert end buffer at the end of songs
const unsigned int START_END_MILLISECONDS = 500;
const auto VERBOSE_FLUIDSYNTH = false;
const auto SECONDS_PER_MINUTE = 60;
const auto NUMBER_OF_MIDI_CHANNELS = 16;
const auto DEFAULT_GAIN = 5;

auto Player::beat_time() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

auto Player::has_real_time() const -> bool {
  return audio_driver_pointer != nullptr;
}

void Player::initialize_play() {
  current_key = song_pointer->starting_key;
  current_volume = song_pointer->starting_volume / PERCENT;
  current_tempo = song_pointer->starting_tempo;

  Q_ASSERT(sequencer_pointer != nullptr);
  starting_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_time = starting_time;
  current_instrument_pointer = song_pointer->starting_instrument_pointer;
}

void Player::modulate(const Chord &chord) {
  current_key = current_key * chord.interval.ratio();
  current_volume = current_volume * chord.volume_ratio.ratio();
  current_tempo = current_tempo * chord.tempo_ratio.ratio();
  const auto &chord_instrument_pointer = chord.instrument_pointer;
  Q_ASSERT(chord_instrument_pointer != nullptr);
  if (!chord_instrument_pointer->instrument_name.empty()) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

auto Player::play_notes(size_t chord_index, const Chord &chord,
                        size_t first_note_index, size_t number_of_notes)
    -> unsigned int {
  const auto &notes = chord.notes;
  unsigned int final_time = 0;
  auto notes_size = notes.size();
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    Q_ASSERT(note_index < notes_size);
    const auto &note = notes[note_index];

    const auto &note_instrument_pointer = note.instrument_pointer;

    Q_ASSERT(note_instrument_pointer != nullptr);
    const auto &instrument_pointer =
        (note_instrument_pointer->instrument_name.empty()
             ? current_instrument_pointer
             : note_instrument_pointer);

    Q_ASSERT(CONCERT_A_FREQUENCY != 0);
    auto key_float =
        HALFSTEPS_PER_OCTAVE *
            log2(current_key * note.interval.ratio() / CONCERT_A_FREQUENCY) +
        CONCERT_A_MIDI;
    auto closest_key = round(key_float);
    auto int_closest_key = static_cast<int16_t>(closest_key);

    auto channel_number = -1;
    for (size_t channel_index = 0; channel_index < NUMBER_OF_MIDI_CHANNELS;
         channel_index = channel_index + 1) {
      Q_ASSERT(channel_index < channel_schedules.size());
      if (current_time >= channel_schedules[channel_index]) {
        channel_number = static_cast<int>(channel_index);
        break;
      }
    }

    if (channel_number == -1) {
      std::stringstream warning_message;
      warning_message << "Out of midi channels for chord " << chord_index + 1
                      << ", note " << note_index + 1 << ". Not playing note.";
      QMessageBox::warning(nullptr, QObject::tr("Playback error error"),
                           QObject::tr(warning_message.str().c_str()));
    } else {
      Q_ASSERT(current_time >= 0);
      auto int_current_time = static_cast<unsigned int>(current_time);

      Q_ASSERT(instrument_pointer != nullptr);
      Q_ASSERT(event_pointer != nullptr);
      fluid_event_program_select(event_pointer, channel_number, soundfont_id,
                                 instrument_pointer->bank_number,
                                 instrument_pointer->preset_number);

      Q_ASSERT(sequencer_pointer != nullptr);
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time, 1);

      fluid_event_pitch_bend(
          event_pointer, channel_number,
          static_cast<int>((key_float - closest_key + ZERO_BEND_HALFSTEPS) *
                           BEND_PER_HALFSTEP));

      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time + 1, 1);

      auto new_volume = current_volume * note.volume_ratio.ratio();
      if (new_volume > 1) {
        std::stringstream warning_message;
        warning_message << "Volume exceeds 100% for chord " << chord_index + 1
                        << ", note " << note_index + 1
                        << ". Playing with 100% volume.";
        QMessageBox::warning(nullptr, QObject::tr("Playback error error"),
                             QObject::tr(warning_message.str().c_str()));
        new_volume = 1;
      }

      fluid_event_noteon(event_pointer, channel_number, int_closest_key,
                         static_cast<int16_t>(new_volume * MAX_VELOCITY));
      fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                              int_current_time + 2, 1);

      auto time_step =
          (beat_time() * note.beats.ratio() * note.tempo_ratio.ratio()) *
          MILLISECONDS_PER_SECOND;
      Q_ASSERT(time_step >= 0);
      const unsigned int end_time =
          int_current_time + static_cast<unsigned int>(time_step);

      fluid_event_noteoff(event_pointer, channel_number, int_closest_key);
      fluid_sequencer_send_at(sequencer_pointer, event_pointer, end_time, 1);

      Q_ASSERT(0 <= channel_number);
      Q_ASSERT(static_cast<size_t>(channel_number) < channel_schedules.size());
      channel_schedules[channel_number] = end_time;

      if (end_time > final_time) {
        final_time = end_time;
      }
    }
  }
  return final_time;
}

auto Player::play_chords(size_t first_chord_index, size_t number_of_chords,
                         int wait_frames) -> unsigned int {
  const auto &chords = song_pointer->chords;
  current_time = current_time + wait_frames;
  unsigned int final_time = 0;
  auto chords_size = chords.size();
  for (auto chord_index = first_chord_index;
       chord_index < first_chord_index + number_of_chords;
       chord_index = chord_index + 1) {
    Q_ASSERT(chord_index < chords_size);
    const auto &chord = chords[chord_index];

    modulate(chord);
    auto end_time = play_notes(chord_index, chord, 0, chord.notes.size());
    if (end_time > final_time) {
      final_time = end_time;
    }
    auto time_step =
        (beat_time() * chord.beats.ratio()) * MILLISECONDS_PER_SECOND;
    current_time = current_time + static_cast<unsigned int>(time_step);
  }
  return final_time;
}

Player::Player(Song *song_pointer)
    : song_pointer(song_pointer),
      channel_schedules(std::vector<unsigned int>(NUMBER_OF_MIDI_CHANNELS, 0)),
      current_instrument_pointer(get_instrument_pointer("")) {

  Q_ASSERT(settings_pointer != nullptr);
  auto cores = std::thread::hardware_concurrency();
  if (cores > 0) {
    fluid_settings_setint(settings_pointer, "synth.cpu-cores",
                          static_cast<int>(cores));
  }
  fluid_settings_setnum(settings_pointer, "synth.gain", DEFAULT_GAIN);
  if (VERBOSE_FLUIDSYNTH) {
    fluid_settings_setint(settings_pointer, "synth.verbose", 1);
  }

  synth_pointer = new_fluid_synth(settings_pointer);
  Q_ASSERT(synth_pointer != nullptr);

  Q_ASSERT(sequencer_pointer != nullptr);
  sequencer_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);
  Q_ASSERT(sequencer_id != -1);

  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath(SOUNDFONT_RELATIVE_PATH)
                            .toStdString();
  Q_ASSERT(std::filesystem::exists(soundfont_file));

  auto maybe_soundfont_id =
      fluid_synth_sfload(synth_pointer, soundfont_file.c_str(), 1);
  Q_ASSERT(maybe_soundfont_id != -1);
  soundfont_id = maybe_soundfont_id;

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
}

Player::~Player() {
  if (has_real_time()) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }

  Q_ASSERT(event_pointer != nullptr);
  delete_fluid_event(event_pointer);

  Q_ASSERT(sequencer_pointer != nullptr);
  delete_fluid_sequencer(sequencer_pointer);

  Q_ASSERT(synth_pointer != nullptr);
  delete_fluid_synth(synth_pointer);

  Q_ASSERT(settings_pointer != nullptr);
  delete_fluid_settings(settings_pointer);
}

void Player::start_real_time(const std::string &driver) {
  if (has_real_time()) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }

  Q_ASSERT(settings_pointer != nullptr);
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 1);
  fluid_settings_setstr(settings_pointer, "audio.driver", driver.c_str());

  Q_ASSERT(synth_pointer != nullptr);
#ifndef NO_SPEAKERS
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif
  if (audio_driver_pointer == nullptr) {
    qWarning("Cannot start audio driver \"%s\"", driver.c_str());
  }
}

auto Player::get_playback_volume() const -> float {
  Q_ASSERT(synth_pointer != nullptr);
  return fluid_synth_get_gain(synth_pointer);
}

void Player::set_playback_volume(float new_value) {
  Q_ASSERT(synth_pointer != nullptr);
  fluid_synth_set_gain(synth_pointer, new_value);
}

void Player::play(int parent_number, size_t first_child_number,
                  size_t number_of_children) {
  initialize_play();
  const auto &chords = song_pointer->chords;
  auto chords_size = chords.size();
  if (parent_number == -1) {
    for (size_t chord_index = 0;
         chord_index < static_cast<size_t>(first_child_number);
         chord_index = chord_index + 1) {
      Q_ASSERT(static_cast<size_t>(chord_index) < chords_size);
      modulate(chords[chord_index]);
    }
    play_chords(first_child_number, number_of_children);
  } else {
    Q_ASSERT(parent_number >= 0);
    auto unsigned_parent_number = static_cast<size_t>(parent_number);
    for (size_t chord_index = 0; chord_index < unsigned_parent_number;
         chord_index = chord_index + 1) {
      Q_ASSERT(chord_index < chords_size);
      modulate(chords[chord_index]);
    }

    Q_ASSERT(unsigned_parent_number < chords_size);
    const auto &chord = chords[unsigned_parent_number];
    modulate(chord);
    play_notes(parent_number, chord, first_child_number, number_of_children);
  }
}

void Player::stop_playing() {
  Q_ASSERT(sequencer_pointer != nullptr);
  fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);

  for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
       channel_number = channel_number + 1) {
    Q_ASSERT(event_pointer != nullptr);
    fluid_event_all_sounds_off(event_pointer, channel_number);

    Q_ASSERT(sequencer_pointer != nullptr);
    fluid_sequencer_send_now(sequencer_pointer, event_pointer);
  }
}

void Player::export_to_file(const std::string &output_file) {
  stop_playing();

  if (has_real_time()) {
    delete_fluid_audio_driver(audio_driver_pointer);
  }

  Q_ASSERT(settings_pointer != nullptr);
  fluid_settings_setstr(settings_pointer, "audio.driver", "file");
  fluid_settings_setstr(settings_pointer, "audio.file.name",
                        output_file.c_str());
  fluid_settings_setint(settings_pointer, "synth.lock-memory", 0);

  initialize_play();
  auto final_time =
      play_chords(0, song_pointer->chords.size(), START_END_MILLISECONDS);

  Q_ASSERT(synth_pointer != nullptr);
#ifndef NO_SPEAKERS
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
#endif

  auto time_step = (final_time - starting_time + START_END_MILLISECONDS) *
                   MILLISECONDS_PER_SECOND;
  Q_ASSERT(time_step >= 0);
  QThread::usleep(static_cast<uint64_t>(time_step));
  stop_playing();

  start_real_time();
}