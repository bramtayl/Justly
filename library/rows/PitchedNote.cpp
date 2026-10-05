#include "rows/PitchedNote.hpp"

#include "column_numbers/PitchedNoteColumn.hpp"
#include "rows/PitchedVoice.hpp"
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

namespace {

void warn_frequency(QWidget& parent, const double frequency,
                    const int chord_number, const int note_number,
                    const QString& comparison, const double limit) {
  QString message;
  QTextStream stream(&message);
  stream << QObject::tr("Frequency ") << QString::number(frequency, 'g', 3);
  add_note_location<PitchedNote>(stream, chord_number, note_number);
  stream << comparison << QString::number(limit, 'g', 3);
  QMessageBox::warning(&parent, QObject::tr("Frequency error"), message);
}

}  // namespace

void PitchedNote::from_xml(xmlNode& node) {
  for (auto& field_node : get_xml_children(node)) {
    const auto name = get_xml_name(field_node);
    if (name == "interval") {
      set_interval_from_xml(interval, field_node);
    } else {
      note_field_from_xml(name, field_node);
    }
  }
}

auto PitchedNote::get_clipboard_schema() -> const char* {
  return "pitched_notes_clipboard.xsd";
}

auto PitchedNote::get_xml_field_name() -> const char* { return "pitched_note"; }

auto PitchedNote::get_number_of_columns() -> int {
  return static_cast<int>(PitchedNoteColumn::number_of_pitched_note_columns);
}

auto PitchedNote::get_column_name(int column_number) -> const char* {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
    case PitchedNoteColumn::number_of_pitched_note_columns:
      Q_UNREACHABLE();
    case PitchedNoteColumn::pitched_note_voice_name_column:
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

auto PitchedNote::get_cells_mime() -> const char* {
  return "application/prs.pitched_notes_cells+xml";
}

auto PitchedNote::is_column_editable(int /*column_number*/) -> bool {
  return true;
}

auto PitchedNote::get_pitched() -> const char* { return "pitched"; }

auto PitchedNote::get_closest_midi(
    QWidget& parent, Player& player,
    const QList<UnpitchedVoice>& /*unpitched_voices*/, const int channel_number,
    const int chord_number, const int note_number) const
    -> std::optional<short> {
  const auto& play_state = player.play_state;
  auto& event = player.event;
  const auto frequency = play_state.current_key * interval_to_double(interval);
  static const auto minimum_frequency =
      midi_number_to_frequency(0 - QUARTER_STEP);
  if (frequency < minimum_frequency) {
    warn_frequency(parent, frequency, chord_number, note_number,
                   QObject::tr(" less than minimum frequency "),
                   minimum_frequency);
    return {};
  }

  if (frequency >= MAX_FREQUENCY) {
    warn_frequency(parent, frequency, chord_number, note_number,
                   QObject::tr(" greater than or equal to maximum frequency "),
                   MAX_FREQUENCY);
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
    const QList<PitchedVoice>& pitched_voices,
    const QList<UnpitchedVoice>& /*unpitched_voices*/) const -> const Program& {
  return get_voice_program(get_some_programs(true),
                           get_voice(pitched_voices, voice_name));
}

auto PitchedNote::get_voice_velocity_ratio(
    const QList<PitchedVoice>& pitched_voices,
    const QList<UnpitchedVoice>& /*unpitched_voices*/) const
    -> const Rational& {
  return get_voice(pitched_voices, voice_name).velocity_ratio;
}

auto PitchedNote::get_data(const int column_number) const -> QVariant {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
    case PitchedNoteColumn::number_of_pitched_note_columns:
      Q_UNREACHABLE();
    case PitchedNoteColumn::pitched_note_voice_name_column:
      return voice_name;
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

void PitchedNote::set_data(const int column_number, const QVariant& new_value) {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
    case PitchedNoteColumn::number_of_pitched_note_columns:
      Q_UNREACHABLE();
    case PitchedNoteColumn::pitched_note_voice_name_column:
      voice_name = variant_to<QString>(new_value);
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

void PitchedNote::column_to_xml(xmlNode& node, const int column_number) const {
  switch (static_cast<PitchedNoteColumn>(column_number)) {
    case PitchedNoteColumn::number_of_pitched_note_columns:
      Q_UNREACHABLE();
    case PitchedNoteColumn::pitched_note_voice_name_column:
      set_xml_string(node, "voice_name", voice_name.toStdString());
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
