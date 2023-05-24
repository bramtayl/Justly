#include "Player.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <qstring.h>         // for QString
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtextstream.h>     // for QTextStream

#include <memory>  // for unique_ptr
#include <set>     // for operator!=, set, _Rb_tree_const_iterator
#include <string>  // for string
#include <vector>  // for vector

#include "Instruments.h"  // for INSTRUMENTS
#include "NoteChord.h"    // for NoteChord
#include "TreeNode.h"     // for TreeNode
class QModelIndex;        // lines 15-15

void Player::modulate(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  key = key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_ratio;
  current_tempo = current_tempo * note_chord_pointer->tempo_ratio;
}

auto Player::get_beat_duration() const -> double {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::add_instrument(QTextStream &csound_io,
                            const std::string &instrument) {
  csound_io << "        instr ";
  csound_io << instrument.c_str();
  csound_io << "\n            a_oscilator STK";
  csound_io << instrument.c_str();
  csound_io
      << " p4, p5\n            outs a_oscilator, a_oscilator\n        endin\n";
}

void Player::schedule_note(QTextStream &csound_io, const TreeNode &node) const {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto instrument = note_chord_pointer->instrument;
  csound_io << "        i \"";
  csound_io << instrument.toStdString().c_str();
  csound_io << "\" ";
  csound_io << current_time;
  csound_io << " ";
  csound_io << get_beat_duration() * note_chord_pointer->beats *
                   note_chord_pointer->tempo_ratio;
  csound_io << " ";
  csound_io << key * node.get_ratio();
  csound_io << " ";
  csound_io << current_volume * note_chord_pointer->volume_ratio;
  csound_io << "\n";
}

void Player::play(const Song &song, const QModelIndex &first_index,
                  size_t rows) {
  
  if (csound_file.open()) {
    // file.fileName() returns the unique file name
    QTextStream csound_io(&csound_file);

    csound_io
        << "<CsoundSynthesizer>\n    <CsOptions>\n        "
           "--output=devaudio\n    </CsOptions>\n    <CsInstruments>\n     "
           "   nchnls = 2\n        0dbfs = 1\n";
    for (const auto &instrument : INSTRUMENTS) {
      add_instrument(csound_io, instrument);
    }
    csound_io << "    </CsInstruments>\n    <CsScore>\n";

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
    if (level == 1) {
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
    } else if (level == 2) {
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

    csound_io << "    </CsScore>\n</CsoundSynthesizer>\n";
    csound_file.close();
  
    csound_data.stop_song();
    csound_data.start_song(csound_file.fileName());
  }
}
