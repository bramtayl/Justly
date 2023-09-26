#include "main/Player.h"

#include <csound/csound.h>            // for csoundSetRTAudioModule, csoundG...
#include <qbytearray.h>        // for QByteArray
#include <qcoreapplication.h>  // for QCoreApplication
#include <qdir.h>              // for QDir
#include <qglobal.h>           // for qCritical
#include <qiodevicebase.h>     // for QIODeviceBase, QIODeviceBase::O...
#include <qstring.h>           // for QString
#include <qtcoreexports.h>     // for qUtf8Printable
#include <qtextstream.h>       // for QTextStream, operator<<, endl

#include <cmath>                    // for log2
#include <csound/csPerfThread.hpp>  // for CsoundPerformanceThread
#include <cstddef>                  // for size_t
#include <memory>                   // for unique_ptr, operator!=, default...
#include <vector>                   // for vector

#include "main/Song.h"             // for Song, FULL_NOTE_VOLUME, SECONDS...
#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument
#include "notechord/NoteChord.h"   // for NoteChord, chord_level, root_level

Player::Player(Song *song_pointer_input, const QString &output_file)
    : song_pointer(song_pointer_input) {
  // only print warnings
  // comment out to debug
  SetOption("--messagelevel=16");

  auto executable_folder = QDir(QCoreApplication::applicationDirPath());

  LoadPlugins(
      qUtf8Printable(executable_folder.filePath(PLUGINS_RELATIVE_PATH)));

  auto soundfont_file = executable_folder.filePath(SOUNDFONT_RELATIVE_PATH);

  if (output_file == "") {
    csoundSetRTAudioModule(GetCsound(), REALTIME_PROVIDER);
    const int number_of_devices =
        csoundGetAudioDevList(GetCsound(), nullptr, 1);
    if (number_of_devices == 0) {
      qCritical("No audio devices!");
      return;
    }
    SetOutput("devaudio", nullptr, nullptr);
    performer_pointer = std::make_unique<CsoundPerformanceThread>(this);
  } else {
    SetOutput(qUtf8Printable(output_file), "wav", nullptr);
  }

  const auto &instruments = Instrument::get_all_instruments();

  QByteArray orchestra_code = "";
  QTextStream orchestra_io(&orchestra_code, QIODeviceBase::WriteOnly);
  orchestra_io << R"(
nchnls = 2
0dbfs = 1

gisound_font sfload ")"
               << soundfont_file << R"("
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
                 << ", gisound_font, " << instrument.instrument_id << Qt::endl;
  }
  orchestra_io.flush();
  CompileOrc(orchestra_code.data());
  if (performer_pointer != nullptr) {
    Start();
  }
}

Player::~Player() {
  if (performer_pointer != nullptr) {
    performer_pointer->Stop();
    performer_pointer->Join();
  }
  Reset();
}

void Player::initialize_song() {
  current_key = song_pointer->starting_key;
  current_volume = (FULL_NOTE_VOLUME * song_pointer->starting_volume) / PERCENT;
  current_tempo = song_pointer->starting_tempo;
  current_time = 0.0;
  current_instrument = song_pointer->starting_instrument;
}

void Player::update_with_chord(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  current_key = current_key * node.get_ratio();
  current_volume =
      current_volume * note_chord_pointer->volume_percent / PERCENT;
  current_tempo = current_tempo * note_chord_pointer->tempo_percent / PERCENT;
  auto maybe_chord_instrument = note_chord_pointer->instrument;
  if (maybe_chord_instrument.instrument_name != "") {
    current_instrument = maybe_chord_instrument;
  }
}

void Player::move_time(const TreeNode &node) {
  current_time =
      current_time + get_beat_duration() * node.note_chord_pointer->beats;
}

auto Player::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::write_note(QTextStream &output_stream,
                        const TreeNode &node) const {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto maybe_instrument = note_chord_pointer->instrument;
  auto instrument = current_instrument;
  if (maybe_instrument.instrument_name != "") {
    instrument = maybe_instrument;
  }
  auto frequency = current_key * node.get_ratio();
  output_stream << "i \"play_soundfont\" " << current_time << " "
                << get_beat_duration() * note_chord_pointer->beats *
                       note_chord_pointer->tempo_percent / PERCENT
                << " " << instrument.instrument_id << " " << frequency << " "
                << HALFSTEPS_PER_OCTAVE *
                           log2(frequency / CONCERT_A_FREQUENCY) +
                       CONCERT_A_MIDI
                << " "
                << current_volume * note_chord_pointer->volume_percent /
                       PERCENT;
}

void Player::write_song() {
  QByteArray score_code = "";
  QTextStream score_io(&score_code, QIODeviceBase::WriteOnly);

  initialize_song();
  for (const auto &chord_node_pointer : song_pointer->root.child_pointers) {
    update_with_chord(*chord_node_pointer);
    for (const auto &note_node_pointer : chord_node_pointer->child_pointers) {
      write_note(score_io, *note_node_pointer);
      score_io << Qt::endl;
    }
    move_time(*chord_node_pointer);
  }
  score_io.flush();
  ReadScore(score_code.data());
  Start();
  Perform();
}

void Player::write_chords(int first_index, int number_of_children,
                          const TreeNode &parent_node) {
  if (performer_pointer != nullptr) {
    stop_playing();
    QByteArray score_code = "";
    QTextStream score_io(&score_code, QIODeviceBase::WriteOnly);

    initialize_song();

    auto end_position = first_index + number_of_children;
    if (!(parent_node.verify_child_at(first_index) &&
          parent_node.verify_child_at(end_position - 1))) {
      return;
    }
    auto parent_level = parent_node.get_level();
    if (parent_level == root_level) {
      for (auto chord_index = 0; chord_index < first_index;
           chord_index = chord_index + 1) {
        update_with_chord(*parent_node.child_pointers[chord_index]);
      }
      for (auto chord_index = first_index; chord_index < end_position;
           chord_index = chord_index + 1) {
        auto &chord = *parent_node.child_pointers[chord_index];
        update_with_chord(chord);
        for (const auto &note_node_pointer : chord.child_pointers) {
          write_note(score_io, *note_node_pointer);
          score_io << Qt::endl;
        }
        move_time(chord);
      }
    } else if (parent_level == chord_level) {
      auto &root = *(parent_node.parent_pointer);
      auto &chord_pointers = root.child_pointers;
      auto chord_position = parent_node.get_row();
      for (auto chord_index = 0; chord_index <= chord_position;
           chord_index = chord_index + 1) {
        update_with_chord(*chord_pointers[chord_index]);
      }
      for (auto note_index = first_index; note_index < end_position;
           note_index = note_index + 1) {
        write_note(score_io, *(parent_node.child_pointers[note_index]));
        score_io << Qt::endl;
      }
    }
    score_io.flush();
    ReadScore(score_code.data());
    performer_pointer->Play();
  }
}

void Player::stop_playing() const {
  if (performer_pointer != nullptr) {
    performer_pointer->Pause();
    performer_pointer->SetScoreOffsetSeconds(0);
    performer_pointer->InputMessage("i \"clear_events\" 0 0");
  }
}
