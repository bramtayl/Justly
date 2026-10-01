#include "other/MidiTrackEvent.hpp"

#include <QtCore/QList>

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

namespace {
const auto MIDI_TEMPO_META_TYPE = 0x51U;

void append_meta_header(QByteArray& bytes, unsigned int type,
                        unsigned int length) {
  static const auto MIDI_META_EVENT_PREFIX = 0xFFU;
  bytes.append(static_cast<char>(MIDI_META_EVENT_PREFIX));
  bytes.append(static_cast<char>(type));
  append_variable_length(bytes, length);
}

void append_status(QByteArray& bytes, unsigned int status,
                   unsigned int channel_number) {
  bytes.append(
      static_cast<char>(status | (channel_number & MIDI_CHANNEL_MASK)));
}

void append_data_byte(QByteArray& bytes, unsigned int value) {
  bytes.append(static_cast<char>(value & MIDI_DATA_BYTE_MASK));
}
}  // namespace

void append_meta_event(QByteArray& bytes, unsigned int type,
                       const QByteArray& payload) {
  append_meta_header(bytes, type, static_cast<unsigned int>(payload.size()));
  bytes.append(payload);
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
  static const auto MIDI_TEMPO_PAYLOAD_LENGTH = 3U;
  append_meta_header(track_data, MIDI_TEMPO_META_TYPE,
                     MIDI_TEMPO_PAYLOAD_LENGTH);
  track_data.append(static_cast<char>(
      (microseconds_per_quarter >> (2 * MIDI_BITS_PER_BYTE)) & MIDI_BYTE_MASK));
  track_data.append(static_cast<char>(
      (microseconds_per_quarter >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  track_data.append(
      static_cast<char>(microseconds_per_quarter & MIDI_BYTE_MASK));
}

void TrackNameEventInfo::write(QByteArray& track_data) const {
  static const auto MIDI_TRACK_NAME_META_TYPE = 0x03U;
  append_meta_event(track_data, MIDI_TRACK_NAME_META_TYPE, name.toUtf8());
}

void ProgramChangeEventInfo::write(QByteArray& track_data) const {
  static const auto MIDI_PROGRAM_CHANGE_STATUS = 0xC0U;
  append_status(track_data, MIDI_PROGRAM_CHANGE_STATUS, channel_number);
  append_data_byte(track_data, program_number);
}

void PitchBendEventInfo::write(QByteArray& track_data) const {
  static const auto MIDI_PITCH_BEND_STATUS = 0xE0U;
  append_status(track_data, MIDI_PITCH_BEND_STATUS, channel_number);
  append_data_byte(track_data, bend_14_bit);
  append_data_byte(track_data, bend_14_bit >> MIDI_SEPTET_BITS);
}

void ControlChangeEventInfo::write(QByteArray& track_data) const {
  static const auto MIDI_CONTROL_CHANGE_STATUS = 0xB0U;
  append_status(track_data, MIDI_CONTROL_CHANGE_STATUS, channel_number);
  append_data_byte(track_data, controller);
  append_data_byte(track_data, value);
}

void NoteOnEventInfo::write(QByteArray& track_data) const {
  static const auto MIDI_NOTE_ON_STATUS = 0x90U;
  append_status(track_data, MIDI_NOTE_ON_STATUS, channel_number);
  append_data_byte(track_data, midi_number);
  append_data_byte(track_data, velocity);
}

void NoteOffEventInfo::write(QByteArray& track_data) const {
  static const auto MIDI_NOTE_OFF_STATUS = 0x80U;
  append_status(track_data, MIDI_NOTE_OFF_STATUS, channel_number);
  append_data_byte(track_data, midi_number);
  append_data_byte(track_data, 0);
}

void MidiTrackEvent::write(QByteArray& track_data) const {
  std::visit(
      [&track_data](const auto& event_info) -> void {
        event_info.write(track_data);
      },
      info);
}
