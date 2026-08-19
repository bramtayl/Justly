#pragma once

#include <QtCore/QString>
#include <variant>

class QByteArray;

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

void append_variable_length(QByteArray &bytes, unsigned int value);

void append_meta_event(QByteArray &bytes, unsigned int type,
                       const QByteArray &payload);

void append_track_name_meta(QByteArray &bytes, const QString &name);

void append_control_change(QByteArray &bytes, unsigned int channel_number,
                           unsigned int controller, unsigned int value);

void append_program_change(QByteArray &bytes, unsigned int channel_number,
                           unsigned int program_number);

void append_note_on(QByteArray &bytes, unsigned int channel_number,
                    unsigned int midi_number, unsigned int velocity);

void append_note_off(QByteArray &bytes, unsigned int channel_number,
                     unsigned int midi_number);

void append_pitch_bend(QByteArray &bytes, unsigned int channel_number,
                       unsigned int bend_14_bit);

void append_be16(QByteArray &bytes, unsigned int value);

void append_chunk(QByteArray &output, const char * chunk_id,
                  const QByteArray &chunk_data);

// base of the per-event-kind payload hierarchy; each subclass knows how to
// write only its own bytes, so a MidiTrackEvent never owns a QByteArray of
// its own -- the payload is written straight into the track's shared buffer
// at export time instead of being allocated and copied per event. Dispatch
// goes through std::visit on MidiEventPayload below, never through an
// EventInfo pointer/reference, so this base stays non-polymorphic
struct EventInfo {};

struct TempoEventInfo : EventInfo {
  unsigned int microseconds_per_quarter = 0;

  void write(QByteArray &track_data) const;
};

struct TrackNameEventInfo : EventInfo {
  QString name;

  void write(QByteArray &track_data) const;
};

struct ProgramChangeEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int program_number = 0;

  void write(QByteArray &track_data) const;
};

struct PitchBendEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int bend_14_bit = 0;

  void write(QByteArray &track_data) const;
};

struct ControlChangeEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int controller = 0;
  unsigned int value = 0;

  void write(QByteArray &track_data) const;
};

struct NoteOnEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int midi_number = 0;
  unsigned int velocity = 0;

  void write(QByteArray &track_data) const;
};

struct NoteOffEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int midi_number = 0;

  void write(QByteArray &track_data) const;
};

using MidiEventPayload =
    std::variant<TempoEventInfo, TrackNameEventInfo, ProgramChangeEventInfo,
                 PitchBendEventInfo, ControlChangeEventInfo, NoteOnEventInfo,
                 NoteOffEventInfo>;

// a single channel or meta event, timed in absolute ticks; tie_break orders
// same-tick events (lower first), e.g. so a note-off lands before a note-on
// that starts at the exact same tick, and a program/pitch-bend change lands
// before the note-on it's meant to apply to
struct MidiTrackEvent {
  double tick = 0;
  int tie_break = 0;
  MidiEventPayload info;

  void write(QByteArray &track_data) const;
};
