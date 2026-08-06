#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <algorithm>
#include <cmath>

static const auto MIDI_BITS_PER_BYTE = 8U;
static const auto MIDI_SEPTET_BITS = 7U; // a variable-length-quantity group,
                                         // and each half of a 14-bit pitch
                                         // bend value, is 7 bits wide
static const auto MIDI_DATA_BYTE_MASK = 0x7FU; // a 7-bit data byte, or one
                                               // variable-length-quantity
                                               // septet
static const auto MIDI_CONTINUATION_BIT = 0x80U; // marks a non-final
                                                 // variable-length-quantity
                                                 // byte
static const auto MIDI_CHANNEL_MASK = 0x0FU;
static const auto MIDI_BYTE_MASK = 0xFFU;

static const auto MIDI_META_EVENT_PREFIX = 0xFFU;
static const auto MIDI_END_OF_TRACK_META_TYPE = 0x2FU;
static const auto MIDI_TEMPO_META_TYPE = 0x51U;
static const auto MIDI_TRACK_NAME_META_TYPE = 0x03U;
static const auto MIDI_PROGRAM_CHANGE_STATUS = 0xC0U;
static const auto MIDI_NOTE_ON_STATUS = 0x90U;
static const auto MIDI_NOTE_OFF_STATUS = 0x80U;
static const auto MIDI_PITCH_BEND_STATUS = 0xE0U;
static const auto MIDI_CONTROL_CHANGE_STATUS = 0xB0U;
static const auto MIDI_BANK_SELECT_MSB_CONTROLLER = 0x00U;
// GM2's standard bank-select value for the percussion bank -- distinct from
// this soundfont's own internal SF2 bank number (128, which doesn't even fit
// in a 7-bit MIDI data byte) used to look up drum presets within FluidSynth
static const auto GM2_PERCUSSION_BANK_SELECT_MSB = 120U;

static const auto MIDI_CHUNK_ID_LENGTH = 4;

// a single channel or meta event, timed in absolute ticks; tie_break orders
// same-tick events (lower first), e.g. so a note-off lands before a note-on
// that starts at the exact same tick, and a program/pitch-bend change lands
// before the note-on it's meant to apply to
struct MidiTrackEvent {
  double tick = 0;
  int tie_break = 0;
  QByteArray bytes;
};

[[nodiscard]] static inline auto
encode_variable_length(unsigned int value) -> QByteArray {
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
  QByteArray encoded;
  for (auto index = static_cast<int>(septets.size()) - 1; index >= 0;
       index = index - 1) {
    auto byte = septets.at(index);
    if (index != 0) {
      byte = byte | MIDI_CONTINUATION_BIT;
    }
    encoded.append(static_cast<char>(byte));
  }
  return encoded;
}

[[nodiscard]] static inline auto
make_meta_event(unsigned int type, const QByteArray &payload) -> QByteArray {
  QByteArray bytes;
  bytes.append(static_cast<char>(MIDI_META_EVENT_PREFIX));
  bytes.append(static_cast<char>(type));
  bytes.append(encode_variable_length(static_cast<unsigned int>(payload.size())));
  bytes.append(payload);
  return bytes;
}

[[nodiscard]] static inline auto make_end_of_track_meta() -> QByteArray {
  return make_meta_event(MIDI_END_OF_TRACK_META_TYPE, {});
}

[[nodiscard]] static inline auto
make_tempo_meta(unsigned int microseconds_per_quarter) -> QByteArray {
  QByteArray payload;
  payload.append(static_cast<char>(
      (microseconds_per_quarter >> (2 * MIDI_BITS_PER_BYTE)) &
      MIDI_BYTE_MASK));
  payload.append(static_cast<char>(
      (microseconds_per_quarter >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  payload.append(
      static_cast<char>(microseconds_per_quarter & MIDI_BYTE_MASK));
  return make_meta_event(MIDI_TEMPO_META_TYPE, payload);
}

[[nodiscard]] static inline auto
make_track_name_meta(const QString &name) -> QByteArray {
  return make_meta_event(MIDI_TRACK_NAME_META_TYPE, name.toUtf8());
}

[[nodiscard]] static inline auto
make_bank_select_msb(unsigned int channel_number,
                     unsigned int bank_msb) -> QByteArray {
  QByteArray bytes;
  bytes.append(static_cast<char>(MIDI_CONTROL_CHANGE_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(MIDI_BANK_SELECT_MSB_CONTROLLER));
  bytes.append(static_cast<char>(bank_msb & MIDI_DATA_BYTE_MASK));
  return bytes;
}

[[nodiscard]] static inline auto
make_program_change(unsigned int channel_number,
                    unsigned int program_number) -> QByteArray {
  QByteArray bytes;
  bytes.append(static_cast<char>(MIDI_PROGRAM_CHANGE_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(program_number & MIDI_DATA_BYTE_MASK));
  return bytes;
}

[[nodiscard]] static inline auto
make_note_on(unsigned int channel_number, unsigned int midi_number,
            unsigned int velocity) -> QByteArray {
  QByteArray bytes;
  bytes.append(static_cast<char>(MIDI_NOTE_ON_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(midi_number & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>(velocity & MIDI_DATA_BYTE_MASK));
  return bytes;
}

[[nodiscard]] static inline auto
make_note_off(unsigned int channel_number,
             unsigned int midi_number) -> QByteArray {
  QByteArray bytes;
  bytes.append(static_cast<char>(MIDI_NOTE_OFF_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(midi_number & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>(0));
  return bytes;
}

[[nodiscard]] static inline auto
make_pitch_bend(unsigned int channel_number,
                unsigned int value_14_bit) -> QByteArray {
  QByteArray bytes;
  bytes.append(static_cast<char>(MIDI_PITCH_BEND_STATUS |
                                 (channel_number & MIDI_CHANNEL_MASK)));
  bytes.append(static_cast<char>(value_14_bit & MIDI_DATA_BYTE_MASK));
  bytes.append(static_cast<char>((value_14_bit >> MIDI_SEPTET_BITS) &
                                 MIDI_DATA_BYTE_MASK));
  return bytes;
}

static inline void append_be16(QByteArray &bytes, unsigned int value) {
  bytes.append(
      static_cast<char>((value >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  bytes.append(static_cast<char>(value & MIDI_BYTE_MASK));
}

static inline void append_chunk(QByteArray &output, const char *const chunk_id,
                                const QByteArray &chunk_data) {
  output.append(chunk_id, MIDI_CHUNK_ID_LENGTH);
  const auto length = static_cast<unsigned int>(chunk_data.size());
  output.append(static_cast<char>((length >> (3 * MIDI_BITS_PER_BYTE)) &
                                  MIDI_BYTE_MASK));
  output.append(static_cast<char>((length >> (2 * MIDI_BITS_PER_BYTE)) &
                                  MIDI_BYTE_MASK));
  output.append(
      static_cast<char>((length >> MIDI_BITS_PER_BYTE) & MIDI_BYTE_MASK));
  output.append(static_cast<char>(length & MIDI_BYTE_MASK));
  output.append(chunk_data);
}

[[nodiscard]] static inline auto
build_track_chunk(QList<MidiTrackEvent> events) -> QByteArray {
  std::ranges::stable_sort(events, [](const MidiTrackEvent &first,
                                      const MidiTrackEvent &second) -> auto {
    if (first.tick != second.tick) {
      return first.tick < second.tick;
    }
    return first.tie_break < second.tie_break;
  });
  QByteArray data;
  auto previous_tick = 0U;
  for (const auto &event : events) {
    const auto tick = static_cast<unsigned int>(std::llround(event.tick));
    data.append(encode_variable_length(tick - previous_tick));
    data.append(event.bytes);
    previous_tick = tick;
  }
  data.append(encode_variable_length(0));
  data.append(make_end_of_track_meta());
  return data;
}

// format 1 (multi-track): the first track is conventionally tempo/meta-only,
// with the remaining tracks holding channel events
[[nodiscard]] static inline auto
build_standard_midi_file(unsigned int format, unsigned int ticks_per_quarter,
                         const QList<QList<MidiTrackEvent>> &tracks)
    -> QByteArray {
  QByteArray output;

  QByteArray header;
  append_be16(header, format);
  append_be16(header, static_cast<unsigned int>(tracks.size()));
  append_be16(header, ticks_per_quarter);
  append_chunk(output, "MThd", header);

  for (const auto &track_events : tracks) {
    append_chunk(output, "MTrk", build_track_chunk(track_events));
  }
  return output;
}
