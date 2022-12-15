#include "Player.h"

#include <Gamma/Domain.h>    // for sampleRate
#include <QtCore/qglobal.h>  // for qCritical, qInfo
#include <qdebug.h>          // for QDebug
#include <qthread.h>         // for QThread

#include <memory>  // for unique_ptr, make_unique, operator!=, def...
#include <vector>  // for vector

#include "Instrument.h"  // for Instrument
#include "NoteChord.h"   // for NoteChord
#include "TreeNode.h"    // for TreeNode
#include "portaudio.h"   // for Pa_GetDeviceCount, Pa_Initialize
class QModelIndex;       // lines 17-17

Player::Player() {
  auto error_code = Pa_Initialize();
  if (error_code != 0) {
    qCritical("Port Audio error code %i", error_code);
  }
  if (Pa_GetDeviceCount() == 0) {
    qCritical("No devices");
  } else {
    audio_io_pointer = std::make_unique<gam::AudioIO>(
        FRAMES_PER_BUFFER,
        gam::AudioDevice(gam::AudioDevice::defaultOutput()).defaultSampleRate(),
        gam::Scheduler::audioCB<gam::AudioIOData>, &scheduler, 2, 0);
    gam::sampleRate(audio_io_pointer->fps());
  }
}

void Player::modulate(const TreeNode &node) {
  const auto &note_chord_pointer = node.note_chord_pointer;
  key = key * node.get_ratio();
  current_volume = current_volume * note_chord_pointer->volume_ratio;
  current_tempo = current_tempo * note_chord_pointer->tempo_ratio;
}

auto Player::get_beat_duration() const -> float {
  return SECONDS_PER_MINUTE / current_tempo;
}

void Player::schedule_note(const TreeNode &node) {
  auto *note_chord_pointer = node.note_chord_pointer.get();
  auto instrument = note_chord_pointer->instrument;
  if (instrument_map.count(instrument) != 1) {
    qInfo() << QString(
                   "Instrument %1 not defined; using the default instrument!")
                   .arg(instrument);
    instrument = "default";
  }
  const auto *sound_pointer = instrument_map[instrument];
  sound_pointer->add(scheduler, current_time, key * node.get_ratio(),
                     current_volume * note_chord_pointer->volume_ratio,
                     get_beat_duration() *
                         static_cast<float>(note_chord_pointer->beats) *
                         note_chord_pointer->tempo_ratio);
}

void Player::play(const Song &song, const QModelIndex &first_index, int rows) {
  if (audio_io_pointer != nullptr) {
    key = static_cast<float>(song.frequency);
    current_volume =
        (FULL_NOTE_VOLUME * static_cast<float>(song.volume_percent)) / PERCENT;
    current_tempo = static_cast<float>(song.tempo);
    current_time = 0.0F;

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
            schedule_note(*nibling_pointer);
          }
          current_time =
              current_time +
              get_beat_duration() *
                  static_cast<float>(sibling.note_chord_pointer->beats);
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
        schedule_note(*sibling_pointers[index]);
      }
    } else {
      qCritical("Invalid level %d!", level);
    }
    audio_io_pointer->start();
    while (!(scheduler.empty())) {
      scheduler.reclaim();
      QThread::msleep(WAIT_MILLISECONDS);
    }
    audio_io_pointer->stop();
  }
}
