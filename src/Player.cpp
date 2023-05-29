#include "Player.h"

#include <QtCore/qglobal.h>  // for qCritical, qInfo
#include <qdebug.h>          // for QDebug
#include <qstring.h>         // for QString
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtextstream.h>     // for QTextStream, operator<<, endl

#include <memory>  // for unique_ptr
#include <string>  // for string
#include <vector>  // for vector

#include "Chord.h"       // for CHORD_LEVEL
#include "CsoundData.h"  // for CsoundData
#include "Note.h"        // for NOTE_LEVEL
#include "NoteChord.h"   // for NoteChord
#include "TreeNode.h"    // for TreeNode
class QModelIndex;       // lines 18-18

Player::Player(QString &orchestra_text) : orchestra_text(orchestra_text)  {
}

void Player::modulate(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  key = key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_ratio;
  current_tempo = current_tempo * note_chord_pointer->tempo_ratio;
}

auto Player::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::schedule_note(QTextStream &csound_io, const TreeNode &node) const {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto instrument = note_chord_pointer->instrument;
  QByteArray raw_string = instrument.toLocal8Bit();
  csound_io << "i \"";
  csound_io << raw_string.data();
  csound_io << "\" ";
  csound_io << current_time;
  csound_io << " ";
  csound_io << get_beat_duration() * note_chord_pointer->beats *
                   note_chord_pointer->tempo_ratio;
  csound_io << " ";
  csound_io << key * node.get_ratio();
  csound_io << " ";
  csound_io << current_volume * note_chord_pointer->volume_ratio;
  csound_io << Qt::endl;
}

void Player::play(const Song &song, const QModelIndex &first_index,
                  size_t rows) {
  if (orchestra_file.open()) {
    QTextStream orchestra_io(&orchestra_file);
    orchestra_io << orchestra_text;
    orchestra_file.close();
  } else {
    qCritical("Cannot open orchestra file!");
  }

  if (score_file.open()) {
    qInfo() << score_file.fileName();
    // file.fileName() returns the unique file name
    QTextStream csound_io(&score_file);

    key = song.frequency;
    current_volume = (FULL_NOTE_VOLUME * song.volume_percent) / PERCENT;
    current_tempo = song.tempo;
    current_time = 0.0;

    const auto &item = song.const_node_from_index(first_index);
    auto item_position = item.is_at_row();
    auto end_position = item_position + rows;
    auto &parent = item.get_parent();
    parent.assert_child_at(item_position);
    parent.assert_child_at(end_position - 1);
    auto &sibling_pointers = parent.child_pointers;
    auto level = item.get_level();
    if (level == CHORD_LEVEL) {
      for (auto index = 0; index < end_position; index = index + 1) {
        auto &sibling = *sibling_pointers[index];
        modulate(sibling);
        if (index >= item_position) {
          for (const auto &nibling_pointer : sibling.child_pointers) {
            schedule_note(csound_io, *nibling_pointer);
          }
          current_time = current_time + get_beat_duration() *
                                            sibling.note_chord_pointer->beats;
        }
      }
    } else if (level == NOTE_LEVEL) {
      auto &grandparent = parent.get_parent();
      auto &uncle_pointers = grandparent.child_pointers;
      auto parent_position = parent.is_at_row();
      grandparent.assert_child_at(parent_position);
      for (auto index = 0; index <= parent_position; index = index + 1) {
        modulate(*uncle_pointers[index]);
      }
      for (auto index = item_position; index < end_position;
           index = index + 1) {
        schedule_note(csound_io, *sibling_pointers[index]);
      }
    } else {
      qCritical("Invalid level %d!", level);
    }

    score_file.close();
  } else {
    qCritical("Cannot open score file!");
  }

  csound_data.stop_song();

  QByteArray raw_orchestra_file = orchestra_file.fileName().toLocal8Bit();
  QByteArray raw_score_file = score_file.fileName().toLocal8Bit();
  csound_data.start_song({"csound", "--output=devaudio",
                                        raw_orchestra_file.data(),
                                        raw_score_file.data()});
}
