#include "rows/PitchedNote.hpp"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <QtWidgets/QMessageBox>
#include <cmath>
#include <fluidsynth.h>
#include <fluidsynth/event.h>
#include <fluidsynth/seq.h>
#include <libxml/parser.h>
#include <optional>

#include "cell_types/Interval.hpp"
#include "cell_types/Program.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "rows/Voice.hpp"
#include "sound/FluidEvent.hpp"
#include "sound/FluidSequencer.hpp"
#include "sound/PlayState.hpp"
#include "sound/Player.hpp"

namespace {
const auto CONCERT_A_FREQUENCY = 440;
const auto CONCERT_A_MIDI = 69;
}  // namespace

auto frequency_to_midi_number(const double key) -> double {
  Q_ASSERT(key > 0);
  return (HALFSTEPS_PER_OCTAVE * log2(key / CONCERT_A_FREQUENCY)) +
         CONCERT_A_MIDI;
}

auto midi_number_to_frequency(const double midi_number) -> double {
  return pow(OCTAVE_RATIO,
             (midi_number - CONCERT_A_MIDI) / HALFSTEPS_PER_OCTAVE) *
         CONCERT_A_FREQUENCY;
}

auto to_int(const double value) -> int {
  return static_cast<int>(std::round(value));
}

void send_event_at(FluidSequencer &sequencer, FluidEvent &event,
                   const double time) {
  Q_ASSERT(time >= 0);
  check_fluid_ok(fluid_sequencer_send_at(
      sequencer.internal_pointer, event.internal_pointer,
      static_cast<unsigned int>(std::round(time)), 1));
}

void PitchedNote::from_xml(xmlNode &node) {
  auto *field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "beats") {
      set_rational_from_xml(beats, field_node);
    } else if (name == "velocity_ratio") {
      set_rational_from_xml(velocity_ratio, field_node);
    } else if (name == "words") {
      words = get_qstring_content(field_node);
    } else if (name == "interval") {
      set_interval_from_xml(interval, field_node);
    } else if (name == "voice_number") {
      voice_number = xml_to_int(field_node);
    } else {
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}

auto PitchedNote::get_clipboard_schema() -> const char * {
  return "pitched_notes_clipboard.xsd";
}

auto PitchedNote::get_xml_field_name() -> const char * {
  return "pitched_note";
}

auto PitchedNote::get_number_of_columns() -> int {
  return static_cast<int>(PitchedNoteColumn::number_of_pitched_note_columns);
}

auto PitchedNote::get_column_name(int column_number) -> const char * {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
  case PitchedNoteColumn::number_of_pitched_note_columns:
    Q_UNREACHABLE();
  case PitchedNoteColumn::pitched_note_voice_number_column:
    return "Voice";
  case PitchedNoteColumn::pitched_note_interval_column:
    return "Interval";
  case PitchedNoteColumn::pitched_note_beats_column:
    return "Beats";
  case PitchedNoteColumn::pitched_note_velocity_ratio_column:
    return "Velocity ratio";
  case PitchedNoteColumn::pitched_note_words_column:
    return "Words";
  }
  Q_UNREACHABLE();
}

auto PitchedNote::get_cells_mime() -> const char * {
  return "application/prs.pitched_notes_cells+xml";
}

auto PitchedNote::is_column_editable(int /*column_number*/) -> bool {
  return true;
}

auto PitchedNote::get_pitched() -> const char * { return "pitched"; }

auto PitchedNote::is_pitched() -> bool { return true; }

auto PitchedNote::get_closest_midi(
    QWidget &parent, Player &player,
    const QList<UnpitchedVoice> & /*unpitched_voices*/,
    const int channel_number, const int chord_number,
    const int note_number) const -> std::optional<short> {
  const auto &play_state = player.play_state;
  auto &event = player.event;
  const auto frequency =
      play_state.current_key * interval_to_double(interval);
  static const auto minimum_frequency =
      midi_number_to_frequency(0 - QUARTER_STEP);
  if (frequency < minimum_frequency) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Frequency ")
           << QString::number(frequency, 'g', 3);
    add_note_location<PitchedNote>(stream, chord_number, note_number);
    stream << QObject::tr(" less than minimum frequency ")
           << QString::number(minimum_frequency, 'g', 3);
    QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
    return {};
  }

  if (frequency >= MAX_FREQUENCY) {
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Frequency ")
           << QString::number(frequency, 'g', 3);
    add_note_location<PitchedNote>(stream, chord_number, note_number);
    stream << QObject::tr(" greater than or equal to maximum frequency ")
           << QString::number(MAX_FREQUENCY, 'g', 3);
    QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
    return {};
  }

  const auto midi_float = frequency_to_midi_number(frequency);
  const auto closest_midi = static_cast<short>(round(midi_float));
  fluid_event_pitch_bend(
      event.internal_pointer, channel_number,
      to_int((midi_float - closest_midi + ZERO_BEND_HALFSTEPS) *
             BEND_PER_HALFSTEP));
  send_event_at(player.sequencer, event, play_state.current_time);
  return closest_midi;
}

auto PitchedNote::get_program(
    const QList<PitchedVoice> &pitched_voices,
    const QList<UnpitchedVoice> & /*unpitched_voices*/) const
    -> const Program & {
  return get_voice_program(get_some_programs(true), pitched_voices, voice_number);
}

auto PitchedNote::get_voice_velocity_ratio(
    const QList<PitchedVoice> &pitched_voices,
    const QList<UnpitchedVoice> & /*unpitched_voices*/) const
    -> const Rational & {
  return pitched_voices.at(voice_number).velocity_ratio;
}

auto PitchedNote::get_data(const int column_number) const -> QVariant {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
  case PitchedNoteColumn::number_of_pitched_note_columns:
    Q_UNREACHABLE();
  case PitchedNoteColumn::pitched_note_voice_number_column:
    return voice_number;
  case PitchedNoteColumn::pitched_note_interval_column:
    return QVariant::fromValue(interval);
  case PitchedNoteColumn::pitched_note_beats_column:
    return QVariant::fromValue(beats);
  case PitchedNoteColumn::pitched_note_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  case PitchedNoteColumn::pitched_note_words_column:
    return words;
  }
  Q_UNREACHABLE();
}

void PitchedNote::set_data(const int column_number, const QVariant &new_value) {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
  case PitchedNoteColumn::number_of_pitched_note_columns:
    Q_UNREACHABLE();
  case PitchedNoteColumn::pitched_note_voice_number_column:
    voice_number = variant_to<int>(new_value);
    break;
  case PitchedNoteColumn::pitched_note_interval_column:
    interval = variant_to<Interval>(new_value);
    break;
  case PitchedNoteColumn::pitched_note_beats_column:
    beats = variant_to<Rational>(new_value);
    break;
  case PitchedNoteColumn::pitched_note_velocity_ratio_column:
    velocity_ratio = variant_to<Rational>(new_value);
    break;
  case PitchedNoteColumn::pitched_note_words_column:
    words = variant_to<QString>(new_value);
    break;
  }
}

void PitchedNote::copy_column_from(const PitchedNote &template_row,
                                   const int column_number) {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
  case PitchedNoteColumn::number_of_pitched_note_columns:
    Q_UNREACHABLE();
  case PitchedNoteColumn::pitched_note_voice_number_column:
    voice_number = template_row.voice_number;
    break;
  case PitchedNoteColumn::pitched_note_interval_column:
    interval = template_row.interval;
    break;
  case PitchedNoteColumn::pitched_note_beats_column:
    beats = template_row.beats;
    break;
  case PitchedNoteColumn::pitched_note_velocity_ratio_column:
    velocity_ratio = template_row.velocity_ratio;
    break;
  case PitchedNoteColumn::pitched_note_words_column:
    words = template_row.words;
    break;
  }
}

void PitchedNote::column_to_xml(xmlNode &node, const int column_number) const {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
  case PitchedNoteColumn::number_of_pitched_note_columns:
    Q_UNREACHABLE();
  case PitchedNoteColumn::pitched_note_voice_number_column:
    set_xml_int(node, "voice_number", voice_number);
    break;
  case PitchedNoteColumn::pitched_note_interval_column:
    maybe_add_interval_to_xml(node, "interval", interval);
    break;
  case PitchedNoteColumn::pitched_note_beats_column:
    maybe_add_rational_to_xml(node, "beats", beats);
    break;
  case PitchedNoteColumn::pitched_note_velocity_ratio_column:
    maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
    break;
  case PitchedNoteColumn::pitched_note_words_column:
    maybe_add_qstring_to_xml(node, "words", words);
    break;
  }
}

void PitchedNote::to_xml(xmlNode &node) const {
  set_xml_int(node, "voice_number", voice_number);
  maybe_add_interval_to_xml(node, "interval", interval);
  maybe_add_rational_to_xml(node, "beats", beats);
  maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
  maybe_add_qstring_to_xml(node, "words", words);
}
