#include "justly/Player.h"

#include <fluidsynth.h>        // for fluid_sequencer_send_at, delete_fluid_...
#include <qbytearray.h>        // for QByteArray
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qstring.h>           // for QString
#include <qtcoreexports.h>     // for qUtf8Printable

#include <cmath>    // for log2, round
#include <cstddef>  // for size_t
#include <cstdint>  // for int16_t
#include <memory>   // for unique_ptr, allocator_traits<>::value_...
#include <string>   // for string
#include <thread>   // for thread
#include <vector>   // for vector

#include "justly/Chord.h"       // for Chord
#include "justly/Instrument.h"  // for Instrument
#include "justly/Interval.h"    // for Interval
#include "justly/Note.h"        // for Note
#include "justly/Song.h"        // for Song

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto MAX_VELOCITY = 127;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto BEND_PER_HALFSTEP = 4096;
const auto ZERO_BEND_HALFSTEPS = 2;

Player::Player(Song *song_pointer_input)
    : current_instrument_pointer(&(Instrument::get_instrument_by_name(""))),
      song_pointer(song_pointer_input),
      synth_pointer(new_fluid_synth(settings_pointer)),
      sequencer_id(fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                                       synth_pointer)) {
  fluid_settings_setint(settings_pointer, "synth.cpu-cores",
                        static_cast<int>(std::thread::hardware_concurrency()));

  soundfont_id = fluid_synth_sfload(
      synth_pointer,
      qUtf8Printable(QDir(QCoreApplication::applicationDirPath())
                         .filePath(SOUNDFONT_RELATIVE_PATH)),
      1);

  fluid_event_set_dest(event_pointer, sequencer_id);

  start_real_time();
}

Player::~Player() {
  delete_fluid_audio_driver(audio_driver_pointer);
  delete_fluid_event(event_pointer);
  delete_fluid_sequencer(sequencer_pointer);
  delete_fluid_synth(synth_pointer);
  delete_fluid_settings(settings_pointer);
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
    const auto &note_pointer = note_pointers[static_cast<size_t>(note_index)];
    const auto &note_instrument_pointer = note_pointer->instrument_pointer;
    const auto &instrument_pointer =
        (note_instrument_pointer->instrument_name.empty()
             ? current_instrument_pointer
             : note_instrument_pointer);

    auto key_float = HALFSTEPS_PER_OCTAVE *
                         log2(current_key * note_pointer->interval.ratio() /
                              CONCERT_A_FREQUENCY) +
                     CONCERT_A_MIDI;
    auto closest_key = round(key_float);

    fluid_event_program_select(
        event_pointer, note_index, static_cast<unsigned int>(soundfont_id),
        static_cast<int16_t>(instrument_pointer->bank_number),
        static_cast<int16_t>(instrument_pointer->preset_number));
    fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                            static_cast<unsigned int>(current_time), 1);

    fluid_event_pitch_bend(
        event_pointer, note_index,
        static_cast<int>((key_float - closest_key + ZERO_BEND_HALFSTEPS) *
                         BEND_PER_HALFSTEP));
    fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                            static_cast<unsigned int>(current_time), 1);

    fluid_event_noteon(
        event_pointer, note_index, static_cast<int16_t>(closest_key),
        static_cast<int16_t>(current_volume * note_pointer->volume_percent /
                             PERCENT * MAX_VELOCITY));
    fluid_sequencer_send_at(sequencer_pointer, event_pointer,
                            static_cast<unsigned int>(current_time), 1);

    fluid_event_noteoff(event_pointer, note_index,
                        static_cast<int16_t>(closest_key));
    fluid_sequencer_send_at(
        sequencer_pointer, event_pointer,
        static_cast<unsigned int>(current_time +
                                  (beat_time() * note_pointer->beats *
                                   note_pointer->tempo_percent / PERCENT) *
                                      MILLISECONDS_PER_SECOND),
        1);
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
    const auto *chord_pointer =
        chord_pointers[static_cast<size_t>(chord_index)].get();
    modulate(chord_pointer);
    play_notes(chord_pointer);
    current_time = current_time + (beat_time() * chord_pointer->beats) *
                                      MILLISECONDS_PER_SECOND;
  }
}

void Player::export_to(const std::string &output_file) {
  stop();
  delete_fluid_audio_driver(audio_driver_pointer);
  fluid_settings_setstr(settings_pointer, "audio.driver", "file");
  fluid_settings_setstr(settings_pointer, "audio.file.name",
                        output_file.c_str());
  play_chords();
  audio_driver_pointer =
      new_fluid_audio_driver(settings_pointer, synth_pointer);
  delete_fluid_audio_driver(audio_driver_pointer);
  start_real_time();
}

void Player::play_selection(int first_child_number, int number_of_children,
                            int chord_number) {
  initialize();
  const auto &chord_pointers = song_pointer->chord_pointers;
  if (chord_number == -1) {
    for (auto chord_index = 0; chord_index < first_child_number;
         chord_index = chord_index + 1) {
      modulate(chord_pointers[static_cast<size_t>(chord_index)].get());
    }
    play_chords(first_child_number, number_of_children);
  } else {
    for (auto chord_index = 0; chord_index <= chord_number;
         chord_index = chord_index + 1) {
      modulate(chord_pointers[static_cast<size_t>(chord_index)].get());
    }
    play_notes(chord_pointers[static_cast<size_t>(chord_number)].get(),
               first_child_number, number_of_children);
  }
}
