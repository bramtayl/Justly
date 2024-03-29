#include "src/Player.h"

#include <csound/csound.h>     // for csoundGetAudioDevList, csoundSetR...
#include <qbytearray.h>        // for QByteArray
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qglobal.h>           // for qWarning
#include <qstring.h>           // for QString
#include <qtcoreexports.h>     // for qUtf8Printable

#include <cmath>                    // for log2
#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread
#include <cstddef>                  // for size_t
#include <memory>                   // for unique_ptr, operator!=, make_unique
#include <sstream>                  // for operator<<, basic_ostream, basic_...
#include <string>                   // for char_traits, basic_string, string
#include <vector>                   // for vector

#include "justly/Chord.h"       // for Chord
#include "src/Instrument.h"  // for Instrument
#include "justly/Interval.h"    // for Interval
#include "justly/Note.h"        // for Note
#include "justly/Song.h"        // for Song

const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
const auto HALFSTEPS_PER_OCTAVE = 12;
const auto PERCENT = 100;
const auto SECONDS_PER_MINUTE = 60;

Player::Player(Song *song_pointer_input, const std::string &output_file)
    : current_key(0),
    current_volume(0),
    current_tempo(0),
    current_time(0),
    performer_pointer(nullptr),
    current_instrument_pointer(&(Instrument::get_instrument_by_name(""))),
    song_pointer(song_pointer_input) {
  // only print warnings
  // comment out to debug
  SetOption("--messagelevel=16");

  auto executable_folder = QDir(QCoreApplication::applicationDirPath());

  LoadPlugins(
      qUtf8Printable(executable_folder.filePath(PLUGINS_RELATIVE_PATH)));

  auto soundfont_file = executable_folder.filePath(SOUNDFONT_RELATIVE_PATH);

  if (output_file.empty()) {
    csoundSetRTAudioModule(GetCsound(), REALTIME_PROVIDER);
    const int number_of_devices =
        csoundGetAudioDevList(GetCsound(), nullptr, 1);
    if (number_of_devices == 0) {
      qWarning("No audio devices!");
      return;
    }
    SetOutput("devaudio", nullptr, nullptr);
    performer_pointer = std::make_unique<CsoundPerformanceThread>(this);
  } else {
    SetOutput(output_file.c_str(), "wav", nullptr);
  }

  const auto &instruments = Instrument::get_all_instruments();

  static const auto orchestra_code = [soundfont_file, instruments]() {
    std::stringstream orchestra_io;
    orchestra_io << R"(
nchnls = 2
0dbfs = 1

gisound_font sfload ")"
                 << qUtf8Printable(soundfont_file) << R"("
; because 0dbfs = 1, not 32767, I guess
gibase_amplitude = 1/32767
; velocity is how hard you hit the key (not how loud it is)
gimax_velocity = 127
; short release
girelease_duration = 0.05

; arguments p1 = instrument, p2 = start_time, p3 = duration, p4 = instrument_number, p5 = frequency, p6 = midi number, p7 = amplitude (max 1)
instr play_soundfont
  ; assume velociy is proportional to amplitude
  ; arguments velocity, midi number, amplitude, frequency, preset number, ignore midi flag
  aleft_sound, aright_sound sfplay3 gimax_velocity * p7, p6, gibase_amplitude * p7, p5, p4, 1
  ; arguments start_level, sustain_duration, mid_level, release_duration, end_level
  acutoff_envelope linsegr 1, p3, 1, girelease_duration, 0
  ; cutoff instruments at end of the duration
  aleft_sound_cut = aleft_sound * acutoff_envelope
  aright_sound_cut = aright_sound * acutoff_envelope
  outs aleft_sound_cut, aright_sound_cut
endin

instr clear_events
    turnoff3 nstrnum("play_soundfont")
endin
)";
    for (size_t index = 0; index < instruments.size(); index = index + 1) {
      const auto &instrument = instruments[index];
      orchestra_io << "gifont" << instrument.instrument_id << " sfpreset "
                   << instrument.preset_number << ", " << instrument.bank_number
                   << ", gisound_font, " << instrument.instrument_id
                   << std::endl;
    }
    return orchestra_io.str();
  }();

  CompileOrc(orchestra_code.c_str());
  if (performer_pointer != nullptr) {
    auto start_result = Start();
    if (start_result < 0) {
      performer_pointer = nullptr;
    }
  }
}

Player::~Player() {
  if (performer_pointer != nullptr) {
    performer_pointer->Stop();
    performer_pointer->Join();
  }
  Reset();
}

void Player::initialize() {
  current_key = song_pointer->starting_key;
  current_volume = song_pointer->starting_volume / PERCENT;
  current_tempo = song_pointer->starting_tempo;
  current_time = 0.0;
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
  current_time = current_time + get_beat_duration() * chord_pointer->beats;
}

auto Player::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::write_note(std::stringstream *output_stream_pointer,
                        const Note *note_pointer) const {
  const auto &note_instrument_pointer = note_pointer->instrument_pointer;
  auto frequency = current_key * note_pointer->interval.get_ratio();
  *output_stream_pointer << "i \"play_soundfont\" " << current_time << " "
                         << get_beat_duration() * note_pointer->beats *
                                note_pointer->tempo_percent / PERCENT
                         << " "
                         << (note_instrument_pointer->instrument_name.empty()
                                 ? current_instrument_pointer
                                 : note_instrument_pointer)
                                ->instrument_id
                         << " " << frequency << " "
                         << HALFSTEPS_PER_OCTAVE *
                                    log2(frequency / CONCERT_A_FREQUENCY) +
                                CONCERT_A_MIDI
                         << " "
                         << current_volume * note_pointer->volume_percent /
                                PERCENT
                         << std::endl;
}

void Player::write_song() {
  std::stringstream score_io;

  initialize();
  for (const auto &chord_pointer : song_pointer->chord_pointers) {
    update_with_chord(chord_pointer.get());
    for (const auto &note_pointer : chord_pointer->note_pointers) {
      write_note(&score_io, note_pointer.get());
    }
    move_time(chord_pointer.get());
  }
  ReadScore(score_io.str().c_str());
  Start();
  Perform();
}

void Player::write_chords(int first_child_number, int number_of_children,
                          int chord_number) {
  std::stringstream score_io;

  const auto &chord_pointers = song_pointer->chord_pointers;
  auto end_position = first_child_number + number_of_children;
  if (chord_number == -1) {
    for (auto chord_index = 0; chord_index < first_child_number;
          chord_index = chord_index + 1) {
      update_with_chord(chord_pointers[chord_index].get());
    }
    for (auto chord_index = first_child_number;
          chord_index < first_child_number + number_of_children;
          chord_index = chord_index + 1) {
      const auto &chord_pointer = chord_pointers[chord_index].get();
      update_with_chord(chord_pointer);
      for (const auto &note_pointer : chord_pointer->note_pointers) {
        write_note(&score_io, note_pointer.get());
      }
      move_time(chord_pointer);
    }
  } else {
    for (auto chord_index = 0; chord_index <= chord_number;
          chord_index = chord_index + 1) {
      update_with_chord(chord_pointers[chord_index].get());
    }
    const auto &chord_pointer = chord_pointers[chord_number];
    for (auto note_index = first_child_number; note_index < end_position;
          note_index = note_index + 1) {
      write_note(&score_io, chord_pointer->note_pointers[note_index].get());
      score_io << std::endl;
    }
  }

  if (has_real_time()) {
    stop_playing();
    initialize();
    ReadScore(score_io.str().c_str());
    performer_pointer->Play();
  }
}

void Player::stop_playing() {
  if (has_real_time()) {
    performer_pointer->Pause();
    performer_pointer->SetScoreOffsetSeconds(0);
    performer_pointer->InputMessage("i \"clear_events\" 0 0");
  }
}

auto Player::has_real_time() const -> bool {
  return performer_pointer != nullptr;
}
