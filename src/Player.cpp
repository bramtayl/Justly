#include "Player.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <qbytearray.h>            // for QByteArray
#include <qglobal.h>
#include <qiodevicebase.h>  // for QIODeviceBase::OpenMode, QIODevice...
#include <qstring.h>
#include <qtextstream.h>  // for QTextStream, operator<<, endl

#include <cmath>   // for log2
#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "NoteChord.h"  // for NoteChord
#include "Song.h"       // for Song, FULL_NOTE_VOLUME, SECONDS_PE...
#include "TreeNode.h"   // for TreeNode

Player::Player(Song &song_input, const QString &output, const QString& driver_input, const QString &format)
    : song(song_input) {
  SetOption(qUtf8Printable(QString("--output=%1").arg(output)));
  if (format != "") {
    SetOption(qUtf8Printable(QString("--format=%1").arg(format)));
  }
  if (driver_input != "") {
    SetOption(qUtf8Printable(QString("-+rtaudio=%1").arg(driver_input)));
  }
  // silence messages
  // comment out to debug
  SetOption("--messagelevel=16");
  if (song.found_soundfont_file) {
    auto orchestra_error_code =
        CompileOrc(song_input.get_orchestra_code().data());
    if (orchestra_error_code != 0) {
      qCritical("Cannot compile orchestra, error code %d", orchestra_error_code);
    }
  }
}

void Player::initialize_song() {
  current_key = song.starting_key;
  current_volume = (FULL_NOTE_VOLUME * song.starting_volume) / PERCENT;
  current_tempo = song.starting_tempo;
  current_time = 0.0;
  current_instrument_id = song.get_instrument_id(song.starting_instrument);
}

void Player::update_with_chord(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  current_key = current_key * node.get_ratio();
  current_volume =
      current_volume * note_chord_pointer->volume_percent / PERCENT;
  current_tempo = current_tempo * note_chord_pointer->tempo_percent / PERCENT;
  auto maybe_chord_instrument_name = note_chord_pointer->instrument;
  if (maybe_chord_instrument_name != "") {
    current_instrument_id = song.get_instrument_id(maybe_chord_instrument_name);
  }
}

void Player::move_time(const TreeNode &node) {
  current_time =
      current_time + get_beat_duration() * node.note_chord_pointer->beats;
}

auto Player::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::write_note(QTextStream &output_stream, const TreeNode &node) const {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto maybe_instrument_name = note_chord_pointer->instrument;
  int instrument_id = current_instrument_id;
  if (maybe_instrument_name != "") {
    instrument_id = song.get_instrument_id(maybe_instrument_name);
  }
  auto frequency = current_key * node.get_ratio();
  output_stream << "i \"play_soundfont\" " << current_time << " "
     << get_beat_duration() * note_chord_pointer->beats *
            note_chord_pointer->tempo_percent / PERCENT
     << " " << instrument_id << " " << frequency << " "
     << HALFSTEPS_PER_OCTAVE * log2(frequency / CONCERT_A_FREQUENCY) +
            CONCERT_A_MIDI
     << " " << current_volume * note_chord_pointer->volume_percent / PERCENT;
};

void Player::write_song() {
  if (song.found_soundfont_file) {
    QByteArray score_code = "";
    QTextStream score_io(&score_code, QIODeviceBase::WriteOnly);

    initialize_song();
    for (const auto &chord_node_pointer : song.root.child_pointers) {
      update_with_chord(*chord_node_pointer);
      for (const auto &note_node_pointer : chord_node_pointer->child_pointers) {
        write_note(score_io, *note_node_pointer);
        score_io << Qt::endl;
      }
      move_time(*chord_node_pointer);
    }
    score_io.flush();
    ReadScore(score_code.data());
  }
}

void Player::write_chords(int first_index, int number_of_children,
                     const TreeNode &parent_node) {
  if (song.found_soundfont_file) {
    QByteArray score_code = "";
    QTextStream score_io(&score_code, QIODeviceBase::WriteOnly);

    initialize_song();

    auto end_position = first_index + number_of_children;
    if (!(parent_node.verify_child_at(first_index) &&
          parent_node.verify_child_at(end_position - 1))) {
      return;
    };
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
        write_note(score_io, *parent_node.child_pointers[note_index]);
        score_io << Qt::endl;
      }
    } else {
      error_level(parent_level);
    }
    score_io.flush();
    ReadScore(score_code.data());
  }
}