#include "Player.h"
#include <QtCore/qglobal.h>  // for qCritical
#include <csound/csound.h>   // for csoundCompile, csoundCreate, csoundDestroy
#include <qstring.h>         // for QString
#include <fstream>           // for operator<<, ofstream, basic_ostream
#include <memory>            // for unique_ptr
#include <string>            // for string, basic_string, allocator, char_tr...
#include <vector>            // for vector
#include "NoteChord.h"       // for NoteChord
#include "TreeNode.h"        // for TreeNode
#include <stdio.h>
class QModelIndex;  // lines 13-13


Player::Player() {
  csound_object_pointer = csoundCreate(nullptr);
}

Player::~Player() {
  csoundDestroy(csound_object_pointer);
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

const std::vector<std::string> INSTRUMENTS = {
  "STKBandedWG",
  "STKBeeThree",
  "STKBlowBotl",
  "STKBlowHole",
  "STKBowed",
  "STKBrass",
  "STKClarinet",
  "STKDrummer",
  "STKFlute",
  "STKFMVoices",
  "STKHevyMetl",
  "STKMandolin",
  "STKModalBar",
  "STKMoog",
  "STKPercFlut",
  "STKPlucked",
  "STKResonate",
  "STKRhodey",
  "STKSaxofony",
  "STKShakers",
  "STKSimple",
  "STKSitar",
  "STKStifKarp",
  "STKTubeBell",
  "STKVoicForm",
  "STKWhistle",
  "STKWurley"
};

void Player::add_instrument(std::ofstream &csound_io, const std::string &instrument) {
  csound_io << "        instr ";
  csound_io << instrument;
  csound_io << "\n            a_oscilator ";
  csound_io << instrument;
  csound_io << " p4, p5\n            outs a_oscilator, a_oscilator\n        endin\n";
}

void Player::schedule_note(std::ofstream &csound_io,
                           const TreeNode &node) const {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto instrument = note_chord_pointer->instrument;
  csound_io << "        i \"";
  csound_io << instrument.toStdString();
  csound_io << "\" ";
  csound_io << current_time;
  csound_io << " ";
  csound_io << get_beat_duration() * note_chord_pointer->beats * note_chord_pointer->tempo_ratio;
  csound_io << " ";
  csound_io << key * node.get_ratio();
  csound_io << " ";
  csound_io << current_volume * note_chord_pointer->volume_ratio;
  csound_io << "\n";
}

uintptr_t run_csound(void *csound_data_pointer);

void Player::play(const Song &song, const QModelIndex &first_index, int rows) {
  std::ofstream csound_io;
  csound_io.open(csound_file);
  csound_io << "<CsoundSynthesizer>\n    <CsOptions>\n        --output=devaudio\n    </CsOptions>\n    <CsInstruments>\n        nchnls = 2\n        0dbfs = 1\n";
  for (const auto &instrument : INSTRUMENTS) {
    add_instrument(csound_io, instrument);
  }
  csound_io << "    </CsInstruments>\n    <CsScore>\n";
  
  key = song.frequency;
  current_volume =
      (FULL_NOTE_VOLUME * song.volume_percent) / PERCENT;
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
        current_time =
            current_time +
            get_beat_duration() *
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
  csound_io.close();

  std::vector<const char*> arguments = {"Justly", csound_file.c_str()};

  int compile_error_code = csoundCompile(csound_object_pointer, 2, arguments.data());
  if (compile_error_code == 0) {
    int run_error_code = csoundPerform(csound_object_pointer);
  }
  csoundReset(csound_object_pointer);
}
