#pragma once

#include <fluidsynth.h>        // for new_fluid_event, new_fluid_sequencer2
#include <fluidsynth/types.h>  // for fluid_audio_driver_t, fluid_event_t
#include <qbytearray.h>        // for QByteArray
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qstring.h>           // for QString
#include <qtcoreexports.h>     // for qUtf8Printable

#include <cmath>    // for log2, round
#include <cstdint>  // for int16_t
#include <memory>   // for unique_ptr, allocator_traits<>::value_...
#include <string>   // for string
#include <vector>   // for vector

#include "justly/Chord.h"     // for Chord
#include "justly/Interval.h"  // for Interval
#include "justly/Note.h"      // for Note
#include "justly/Song.h"      // for Song
#include "justly/macros.h"
#include "src/Instrument.h"  // for Instrument

struct Chord;
struct Instrument;
struct Song;

const auto PERCENT = 100;
const auto SECONDS_PER_MINUTE = 60;
const auto NUMBER_OF_MIDI_CHANNELS = 16;

class Player {
  double current_key = 0;
  double current_volume = 0;
  double current_tempo = 0;
  double current_time = 0;
  const Instrument *current_instrument_pointer;
  const Song *song_pointer;
  fluid_event_t *event_pointer = new_fluid_event();
  fluid_settings_t *settings_pointer = new_fluid_settings();
  fluid_synth_t *synth_pointer = nullptr;
  fluid_sequencer_t *sequencer_pointer = new_fluid_sequencer2(0);
  fluid_audio_driver_t *audio_driver_pointer = nullptr;
  fluid_seq_id_t sequencer_id = -1;
  int soundfont_id = -1;

  inline void initialize() {
    current_key = song_pointer->starting_key;
    current_volume = song_pointer->starting_volume / PERCENT;
    current_tempo = song_pointer->starting_tempo;
    current_time = fluid_sequencer_get_tick(sequencer_pointer);
    current_instrument_pointer = song_pointer->starting_instrument_pointer;
  }

  inline void update_with_chord(const Chord *chord_pointer) {
    current_key = current_key * chord_pointer->interval.ratio();
    current_volume = current_volume * chord_pointer->volume_percent / PERCENT;
    current_tempo = current_tempo * chord_pointer->tempo_percent / PERCENT;
    const auto &chord_instrument_pointer = chord_pointer->instrument_pointer;
    if (!chord_instrument_pointer->instrument_name.empty()) {
      current_instrument_pointer = chord_instrument_pointer;
    }
  }

  void play_notes(const Chord *chord_pointer, int first_note_index = 0,
                  int number_of_notes = -1) const;

  [[nodiscard]] inline auto get_beat_duration() const -> double {
    return SECONDS_PER_MINUTE / current_tempo;
  }

 public:
  explicit Player(Song *song_pointer);
  ~Player();
  void play_selection(int first_child_number, int number_of_children,
                      int chord_number);
  void export_to(const std::string &output_file);

  inline void stop() {
    fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);
    for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
         channel_number = channel_number + 1) {
      fluid_event_all_sounds_off(event_pointer, channel_number);
      fluid_sequencer_send_now(sequencer_pointer, event_pointer);
    }
  }

  NO_MOVE_COPY(Player);

 private:
  inline void start_real_time() {
#ifdef __linux__
    fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#endif
    audio_driver_pointer =
        new_fluid_audio_driver(settings_pointer, synth_pointer);
  }
  void play_chords(int first_chord_index = 0, int number_of_chords = -1);
};
