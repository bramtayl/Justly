#include "other/MidiTrackEvent.hpp"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <variant>

namespace {
const auto MIDI_BITS_PER_BYTE = 8U;
const auto MIDI_SEPTET_BITS = 7U;        // a variable-length-quantity group,
                                         // and each half of a 14-bit pitch
                                         // bend value, is 7 bits wide
const auto MIDI_DATA_BYTE_MASK = 0x7FU;  // a 7-bit data byte, or one
                                         // variable-length-quantity
                                         // septet
const auto MIDI_CHANNEL_MASK = 0x0FU;
const auto MIDI_BYTE_MASK = 0xFFU;
}  // namespace

void append_variable_length(QByteArray& bytes, unsigned int value) {
  static const auto MIDI_CONTINUATION_BIT = 0x80U;  // marks a non-final
                                                    // variable-length-quantity
                                                    // byte
  QList<unsigned int> septets;
  septets.push_back(value & MIDI_DATA_BYTE_MASK);
  value = value >> MIDI_SEPTET_BITS;
  while (value > 0) {
    septets.push_back(value & MIDI_DATA_BYTE_MASK);
    value = value >> MIDI_SEPTET_BITS;
  }
  // septets were collected least-significant-first; the MIDI variable-length
  // encoding is written most-significant-first, with the continuation bit
  // set on every byte except the last
  for (auto index = static_cast<int>(septets.size()) - 1; index >= 0;
       index = index - 1) {
    auto byte = septets.at(index);
    if (index != 0) {
      byte = byte | MIDI_CONTINUATION_BIT;
    }
    bytes.append(static_cast<char>(byte));
  }
}

void append_meta_event(QByteArray& bytes, unsigned int type,
                       const QByteArray& payload) {
  static const auto MIDI_META_EVENT_PREFIX = 0xFFU;
  bytes.append(static_cast<char>(MIDI_META_EVENT_PREFIX));
  bytes.append(static_cast<char>(type));
  append_variable_length(bytes, static_cast<unsigned int>(payload.size()));
  bytes.append(payload);
}

void append_track_name_meta(QByteArray& bytes, const QString& name) {
  static const auto MIDI_TRACK_NAME_META_TYPE = 0x03U;
  append_meta_event(bytes, MIDI_TRACK_NAME_META_TYPE, name.toUtf8());
}

void append_control_change(QByteArray& bytes, unsigned int channel_number,
                           unsigned int controller, unsigned int value) {
  static const auto MIDI_CONTROL_CHANGE_STATUS = 0xB0U;
  bytes.append(static_cast<char>(MIDI_CONTROL_CHANGE_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(controller & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>(value & MIDI_DATA_BYTE_MASK));
}

void append_program_change(QByteArray& bytes, unsigned int channel_number,
                           unsigned int program_number) {
  static const auto MIDI_PROGRAM_CHANGE_STATUS = 0xC0U;
  bytes.append(static_cast<char>(MIDI_PROGRAM_CHANGE_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(program_number & MIDI_DATA_BYTE_MASK));
}

void append_note_on(QByteArray& bytes, unsigned int channel_number,
                    unsigned int midi_number, unsigned int velocity) {
  static const auto MIDI_NOTE_ON_STATUS = 0x90U;
  bytes.append(static_cast<char>(MIDI_NOTE_ON_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(midi_number & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>(velocity & MIDI_DATA_BYTE_MASK));
}

void append_note_off(QByteArray& bytes, unsigned int channel_number,
                     unsigned int midi_number) {
  static const auto MIDI_NOTE_OFF_STATUS = 0x80U;
  bytes.append(static_cast<char>(MIDI_NOTE_OFF_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(midi_number & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>(0));
}

void append_pitch_bend(QByteArray& bytes, unsigned int channel_number,
                       unsigned int bend_14_bit) {
  static const auto MIDI_PITCH_BEND_STATUS = 0xE0U;
  bytes.append(static_cast<char>(MIDI_PITCH_BEND_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(bend_14_bit & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>((bend_14_bit >> MIDI_SEPTET_BITS) &
                                 MIDI_DATA_BYTE_MASK));
}

void append_be16(QByteArray& bytes, unsigned int value) {
  bytes.append(
      static_cast<char>((value >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  bytes.append(static_cast<char>(value & MIDI_BYTE_MASK));
}

void append_chunk(QByteArray& output, const char* const chunk_id,
                  const QByteArray& chunk_data) {
  static const auto MIDI_CHUNK_ID_LENGTH = 4;
  output.append(chunk_id, MIDI_CHUNK_ID_LENGTH);
  const auto length = static_cast<unsigned int>(chunk_data.size());
  output.append(
      static_cast<char>((length >> (3 * MIDI_BITS_PER_BYTE)) & MIDI_BYTE_MASK));
  output.append(
      static_cast<char>((length >> (2 * MIDI_BITS_PER_BYTE)) & MIDI_BYTE_MASK));
  output.append(
      static_cast<char>((length >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  output.append(static_cast<char>(length & MIDI_BYTE_MASK));
  output.append(chunk_data);
}

void TempoEventInfo::write(QByteArray& track_data) const {
  QByteArray payload;
  payload.append(static_cast<char>(
      (microseconds_per_quarter >> (2 * MIDI_BITS_PER_BYTE)) & MIDI_BYTE_MASK));
  payload.append(static_cast<char>(
      (microseconds_per_quarter >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  payload.append(static_cast<char>(microseconds_per_quarter & MIDI_BYTE_MASK));
  append_meta_event(track_data, MIDI_TEMPO_META_TYPE, payload);
}

void TrackNameEventInfo::write(QByteArray& track_data) const {
  append_track_name_meta(track_data, name);
}

void ProgramChangeEventInfo::write(QByteArray& track_data) const {
  append_program_change(track_data, channel_number, program_number);
}

void PitchBendEventInfo::write(QByteArray& track_data) const {
  append_pitch_bend(track_data, channel_number, bend_14_bit);
}

void ControlChangeEventInfo::write(QByteArray& track_data) const {
  append_control_change(track_data, channel_number, controller, value);
}

void NoteOnEventInfo::write(QByteArray& track_data) const {
  append_note_on(track_data, channel_number, midi_number, velocity);
}

void NoteOffEventInfo::write(QByteArray& track_data) const {
  append_note_off(track_data, channel_number, midi_number);
}

void MidiTrackEvent::write(QByteArray& track_data) const {
  std::visit(
      [&track_data](const auto& event_info) -> void {
        event_info.write(track_data);
      },
      info);
}
