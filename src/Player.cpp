#include "src/Player.h"

#include <qbytearray.h>        // for QByteArray
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qglobal.h>           // for qWarning
#include <qstring.h>           // for QString
#include <qtcoreexports.h>     // for qUtf8Printable

#include <cmath>    // for log2
#include <cstddef>  // for size_t
#include <cstdint>
#include <memory>   // for unique_ptr, operator!=, make_unique
#include <sstream>  // for operator<<, basic_ostream, basic_...
#include <string>   // for char_traits, basic_string, string
#include <thread>
#include <vector>  // for vector

#include "justly/Chord.h"     // for Chord
#include "justly/Interval.h"  // for Interval
#include "justly/Note.h"      // for Note
#include "justly/Song.h"      // for Song
#include "src/Instrument.h"   // for Instrument

using namespace std::chrono_literals;

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto PERCENT = 100;
const auto SECONDS_PER_MINUTE = 60;
const auto MAX_VELOCITY = 127;
const auto MILLISECONDS_PER_SECOND = 1000;

Player::Player(Song *song_pointer_input)
    : current_key(0),
      current_volume(0),
      current_tempo(0),
      current_time(0),
      current_instrument_pointer(&(Instrument::get_instrument_by_name(""))),
      song_pointer(song_pointer_input) {
  set_up();
}

Player::~Player() {
  kill();
}

void Player::set_up(const std::string &output_file) {
  auto soundfont_file = QDir(QCoreApplication::applicationDirPath())
                            .filePath(SOUNDFONT_RELATIVE_PATH);
  auto *fluid_settings_pointer = new_fluid_settings();
  auto verbose_set_result =
      fluid_settings_setint(fluid_settings_pointer, "synth.verbose", 1);
  if (verbose_set_result == FLUID_FAILED) {
    qWarning("Verbose setting failed");
  }
  auto real_time = output_file.empty();
  if (real_time) {
#ifdef __linux__
    auto driver_set_result = fluid_settings_setstr(
        fluid_settings_pointer, "audio.driver", "pulseaudio");
    if (driver_set_result == FLUID_FAILED) {
      qWarning("Driver setting failed");
    }
#else
    qInfo("Using default audio driver");
#endif
  } else {
    auto driver_set_result = fluid_settings_setstr(
        fluid_settings_pointer, "audio.driver", "file");
    if (driver_set_result == FLUID_FAILED) {
      qWarning("Driver setting failed");
    }

    auto set_file_name_result = fluid_settings_setstr(fluid_settings_pointer, "audio.file.name",
                          output_file.c_str());
    if (set_file_name_result == FLUID_FAILED) {
      qWarning("Setting filename to %s", output_file.c_str());
    }
  }
  auto threads_set_result = fluid_settings_setint(
      fluid_settings_pointer, "synth.cpu-cores",
      static_cast<int>(std::thread::hardware_concurrency()));
  if (threads_set_result == FLUID_FAILED) {
    qWarning("Driver setting failed");
  }
  synth_pointer = new_fluid_synth(fluid_settings_pointer);
  if (synth_pointer == nullptr) {
    qWarning("Failed to create synthesizer!");
  }
  audio_driver_pointer =
      new_fluid_audio_driver(fluid_settings_pointer, synth_pointer);
  if (audio_driver_pointer == nullptr) {
    qWarning("Failed to create audio driver!");
  }
  sequencer_pointer = new_fluid_sequencer2(0);
  synth_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);
  soundfont_id =
      fluid_synth_sfload(synth_pointer, qUtf8Printable(soundfont_file), 1);
  if (soundfont_id == FLUID_FAILED) {
    qWarning("Loading soundfont file %s failed",
             qUtf8Printable(soundfont_file));
  }
}

void Player::initialize() {
  current_key = song_pointer->starting_key;
  current_volume = song_pointer->starting_volume / PERCENT;
  current_tempo = song_pointer->starting_tempo;
  current_time = fluid_sequencer_get_tick(sequencer_pointer);
  current_instrument_pointer = song_pointer->starting_instrument_pointer;
}

void Player::update_with_chord(const Chord *chord_pointer) {
  current_key = current_key * chord_pointer->interval.get_ratio();
  current_volume = current_volume * chord_pointer->volume_percent / PERCENT;
  current_tempo = current_tempo * chord_pointer->tempo_percent / PERCENT;
  const auto &chord_instrument_pointer = chord_pointer->instrument_pointer;
  if (!chord_instrument_pointer->instrument_name.empty()) {
    current_instrument_pointer = chord_instrument_pointer;
  }
}

void Player::move_time(const Chord *chord_pointer) {
  current_time = current_time + (get_beat_duration() * chord_pointer->beats) *
                                    MILLISECONDS_PER_SECOND;
}

auto Player::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::play_notes(const Chord *chord_pointer, int first_note_index,
                        int number_of_notes) const {
  if (number_of_notes == -1) {
    number_of_notes = static_cast<int>(chord_pointer->note_pointers.size());
  }
  const auto &note_pointers = chord_pointer->note_pointers;
  for (auto note_index = first_note_index;
       note_index < first_note_index + number_of_notes;
       note_index = note_index + 1) {
    const auto &note_pointer = note_pointers[note_index];
    const auto &note_instrument_pointer = note_pointer->instrument_pointer;
    const auto &instrument_pointer =
        (note_instrument_pointer->instrument_name.empty()
             ? current_instrument_pointer
             : note_instrument_pointer);

    auto key = static_cast<int16_t>(
        HALFSTEPS_PER_OCTAVE *
            log2(current_key * note_pointer->interval.get_ratio() /
                 CONCERT_A_FREQUENCY) +
        CONCERT_A_MIDI);

    fluid_event_t *instrument_change_event_pointer = new_fluid_event();
    fluid_event_set_dest(instrument_change_event_pointer, synth_id);
    fluid_event_program_select(
        instrument_change_event_pointer, note_index, soundfont_id,
        static_cast<int16_t>(instrument_pointer->bank_number),
        static_cast<int16_t>(instrument_pointer->preset_number));
    auto instrument_change_result = fluid_sequencer_send_at(
        sequencer_pointer, instrument_change_event_pointer,
        static_cast<unsigned int>(current_time), 1);
    if (instrument_change_result == FLUID_FAILED) {
      qWarning("Changing instrument failed for note %d", note_index);
    }

    fluid_event_t *note_on_event_pointer = new_fluid_event();
    fluid_event_set_dest(note_on_event_pointer, synth_id);
    fluid_event_noteon(
        note_on_event_pointer, note_index, key,
        static_cast<int16_t>(current_volume * note_pointer->volume_percent /
                             PERCENT * MAX_VELOCITY));
    auto note_on_send_result =
        fluid_sequencer_send_at(sequencer_pointer, note_on_event_pointer,
                                static_cast<unsigned int>(current_time), 1);
    if (note_on_send_result == FLUID_FAILED) {
      qWarning("Sending note on event failed for note %d", note_index);
    }
    delete_fluid_event(note_on_event_pointer);

    fluid_event_t *note_off_event_pointer = new_fluid_event();
    fluid_event_set_dest(note_off_event_pointer, synth_id);
    fluid_event_noteoff(note_off_event_pointer, note_index, key);
    auto note_off_send_result = fluid_sequencer_send_at(
        sequencer_pointer, note_off_event_pointer,
        static_cast<unsigned int>(current_time +
                                  (get_beat_duration() * note_pointer->beats *
                                   note_pointer->tempo_percent / PERCENT) *
                                      MILLISECONDS_PER_SECOND),
        1);
    if (note_off_send_result == FLUID_FAILED) {
      qWarning("Sending note off event failed for note %d", note_index);
    }
    delete_fluid_event(note_off_event_pointer);
  }
}

void Player::play_chords(int first_chord_index, int number_of_chords) {
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (number_of_chords == -1) {
    number_of_chords = static_cast<int>(chord_pointers.size());
  }
  for (auto chord_index = first_chord_index;
       chord_index < first_chord_index + number_of_chords;
       chord_index = chord_index + 1) {
    const auto *chord_pointer = chord_pointers[chord_index].get();
    update_with_chord(chord_pointer);
    play_notes(chord_pointer);
    move_time(chord_pointer);
  }
}


void Player::write_song(const std::string &output_file) {
  kill();
  set_up(output_file);
  play_chords();
  kill();
  set_up();
}

void Player::play_selection(int first_child_number, int number_of_children,
                            int chord_number) {
  initialize();
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (chord_number == -1) {
    for (auto chord_index = 0; chord_index < first_child_number;
         chord_index = chord_index + 1) {
      update_with_chord(chord_pointers[chord_index].get());
    }
    play_chords(first_child_number, first_child_number + number_of_children);
  } else {
    for (auto chord_index = 0; chord_index <= chord_number;
         chord_index = chord_index + 1) {
      update_with_chord(chord_pointers[chord_index].get());
    }
    play_notes(chord_pointers[chord_number].get(), first_child_number,
               number_of_children);
  }
}

void Player::kill() {
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_audio_driver(audio_driver_pointer);
  delete_fluid_synth(synth_pointer);
}

void Player::stop_playing() {
  kill();
  set_up();
}
