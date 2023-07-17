#include "Performer.h"
#include "src/NoteChord.h"    // for error_level, chord_level, root_level
#include "src/Player.h"       // for Player
#include "src/TreeNode.h"     // for TreeNode
#include <ext/alloc_traits.h> // for __alloc_traits<>::value_type
#include <memory>             // for unique_ptr
#include <qbytearray.h>       // for QByteArray
#include <qiodevicebase.h>    // for QIODeviceBase, QIODeviceBase::OpenMode
#include <qtextstream.h>      // for QTextStream
#include <vector>             // for vector
class Csound;

Performer::Performer(Player &player_input)
    : player(player_input),
      CsoundPerformanceThread(dynamic_cast<Csound *>(&player_input)) {
  player.Start();
};

Performer::~Performer() {
  Stop();
  Join();
}

void Performer::stop_playing() {
  SetScoreOffsetSeconds(0);
  InputMessage("i \"clear_events\" 0 0");
}

void Performer::play(int first_index, int number_of_children,
                     const TreeNode &parent_node) {
  stop_playing();

  player.initialize_song();

  auto end_position = first_index + number_of_children;
  if (!(parent_node.verify_child_at(first_index) &&
        parent_node.verify_child_at(end_position - 1))) {
    return;
  };
  auto parent_level = parent_node.get_level();
  if (parent_level == root_level) {
    for (auto chord_index = 0; chord_index < first_index;
         chord_index = chord_index + 1) {
      player.update_with_chord(*parent_node.child_pointers[chord_index]);
    }
    for (auto chord_index = first_index; chord_index < end_position;
         chord_index = chord_index + 1) {
      auto &chord = *parent_node.child_pointers[chord_index];
      player.update_with_chord(chord);
      for (const auto &note_node_pointer : chord.child_pointers) {
        schedule_note(*note_node_pointer);
      }
      player.move_time(chord);
    }
  } else if (parent_level == chord_level) {
    auto &root = *(parent_node.parent_pointer);
    auto &chord_pointers = root.child_pointers;
    auto chord_position = parent_node.is_at_row();
    for (auto chord_index = 0; chord_index <= chord_position;
         chord_index = chord_index + 1) {
      player.update_with_chord(*chord_pointers[chord_index]);
    }
    for (auto note_index = first_index; note_index < end_position;
         note_index = note_index + 1) {
      schedule_note(*parent_node.child_pointers[note_index]);
    }
  } else {
    error_level(parent_level);
  }

  Play();
}

void Performer::schedule_note(const TreeNode &node) {
  QByteArray note_message;
  QTextStream message_io(&note_message, QIODeviceBase::WriteOnly);
  player.csound_note_event(message_io, node);
  message_io.flush();
  InputMessage(note_message.data());
}