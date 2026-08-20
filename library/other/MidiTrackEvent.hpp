#pragma once

#include <QtCore/QString>

static const auto MIDI_TEMPO_META_TYPE = 0x51U;

void append_variable_length(QByteArray& bytes, unsigned int value);

void append_meta_event(QByteArray& bytes, unsigned int type,
                       const QByteArray& payload);

void append_track_name_meta(QByteArray& bytes, const QString& name);

void append_control_change(QByteArray& bytes, unsigned int channel_number,
                           unsigned int controller, unsigned int value);

void append_program_change(QByteArray& bytes, unsigned int channel_number,
                           unsigned int program_number);

void append_note_on(QByteArray& bytes, unsigned int channel_number,
                    unsigned int midi_number, unsigned int velocity);

void append_note_off(QByteArray& bytes, unsigned int channel_number,
                     unsigned int midi_number);

void append_pitch_bend(QByteArray& bytes, unsigned int channel_number,
                       unsigned int bend_14_bit);

void append_be16(QByteArray& bytes, unsigned int value);

void append_chunk(QByteArray& output, const char* chunk_id,
                  const QByteArray& chunk_data);

// base of the per-event-kind payload hierarchy; each subclass knows how to
// write only its own bytes, so a MidiTrackEvent never owns a QByteArray of
// its own -- the payload is written straight into the track's shared buffer
// at export time instead of being allocated and copied per event. Dispatch
// goes through std::visit on MidiEventPayload below, never through an
// EventInfo pointer/reference, so this base stays non-polymorphic
struct EventInfo {};

struct TempoEventInfo : EventInfo {
  unsigned int microseconds_per_quarter = 0;

  void write(QByteArray& track_data) const;
};

struct TrackNameEventInfo : EventInfo {
  QString name;

  void write(QByteArray& track_data) const;
};

struct ProgramChangeEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int program_number = 0;

  void write(QByteArray& track_data) const;
};

struct PitchBendEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int bend_14_bit = 0;

  void write(QByteArray& track_data) const;
};

struct ControlChangeEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int controller = 0;
  unsigned int value = 0;

  void write(QByteArray& track_data) const;
};

struct NoteOnEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int midi_number = 0;
  unsigned int velocity = 0;

  void write(QByteArray& track_data) const;
};

struct NoteOffEventInfo : EventInfo {
  unsigned int channel_number = 0;
  unsigned int midi_number = 0;

  void write(QByteArray& track_data) const;
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

  void write(QByteArray& track_data) const;
};
