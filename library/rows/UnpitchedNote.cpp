#include "rows/UnpitchedNote.hpp"

#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "rows/UnpitchedVoice.hpp"

void UnpitchedNote::from_xml(
    xmlNode& node, const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& /*unpitched_voices*/) {
  for (auto& field_node : get_xml_children(node)) {
    note_field_from_xml(get_xml_name(field_node), field_node);
  }
}

auto UnpitchedNote::get_clipboard_schema() -> const char* {
  return "unpitched_notes_clipboard.xsd";
}

auto UnpitchedNote::get_xml_field_name() -> const char* {
  return "unpitched_note";
}

auto UnpitchedNote::get_number_of_columns() -> int {
  return static_cast<int>(
      UnpitchedNoteColumn::number_of_unpitched_note_columns);
}

auto UnpitchedNote::get_column_name(int column_number) -> const char* {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_name_column:
      return "Voice";
    case UnpitchedNoteColumn::unpitched_note_beats_column:
      return "Beats";
    case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
      return "Velocity ratio";
    case UnpitchedNoteColumn::unpitched_note_words_column:
      return "Words";
  }
  Q_UNREACHABLE();
}

auto UnpitchedNote::get_cells_mime() -> const char* {
  return "application/prs.unpitched_notes_cells+xml";
}

auto UnpitchedNote::get_pitched() -> const char* { return "unpitched"; }

auto UnpitchedNote::is_column_editable(int /*column_number*/) -> bool {
  return true;
}

auto UnpitchedNote::get_closest_midi(
    QWidget& /*parent*/, Player& /*player*/,
    const QList<UnpitchedVoice>& unpitched_voices, const int /*channel_number*/,
    int /*chord_number*/, int /*note_number*/) const -> std::optional<short> {
  return static_cast<short>(
      get_voice(unpitched_voices, voice_name).midi_number);
}

auto UnpitchedNote::get_program(
    const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& unpitched_voices) const -> const Program& {
  return get_voice_program(get_some_programs(false),
                           get_voice(unpitched_voices, voice_name));
}

auto UnpitchedNote::get_voice_velocity_ratio(
    const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& unpitched_voices) const -> const Rational& {
  return get_voice(unpitched_voices, voice_name).velocity_ratio;
}

auto UnpitchedNote::get_data(const int column_number) const -> QVariant {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_name_column:
      return voice_name;
    case UnpitchedNoteColumn::unpitched_note_beats_column:
      return QVariant::fromValue(beats);
    case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    case UnpitchedNoteColumn::unpitched_note_words_column:
      return words;
  }
  Q_UNREACHABLE();
}

void UnpitchedNote::set_data(const int column_number,
                             const QVariant& new_value) {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_name_column:
      voice_name = variant_to<QString>(new_value);
      break;
    case UnpitchedNoteColumn::unpitched_note_beats_column:
      beats = variant_to<Rational>(new_value);
      break;
    case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case UnpitchedNoteColumn::unpitched_note_words_column:
      words = variant_to<QString>(new_value);
      break;
  }
}

void UnpitchedNote::copy_column_from(const UnpitchedNote& template_row,
                                     const int column_number) {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_name_column:
      voice_name = template_row.voice_name;
      break;
    case UnpitchedNoteColumn::unpitched_note_beats_column:
      beats = template_row.beats;
      break;
    case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    case UnpitchedNoteColumn::unpitched_note_words_column:
      words = template_row.words;
      break;
  }
}

void UnpitchedNote::column_to_xml(
    xmlNode& node, const int column_number,
    const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& /*unpitched_voices*/) const {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_name_column:
      set_xml_string(node, "voice_name", voice_name.toStdString());
      break;
    case UnpitchedNoteColumn::unpitched_note_beats_column:
      maybe_add_rational_to_xml(node, "beats", beats);
      break;
    case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
    case UnpitchedNoteColumn::unpitched_note_words_column:
      maybe_add_qstring_to_xml(node, "words", words);
      break;
  }
}

void UnpitchedNote::to_xml(
    xmlNode& node, const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& /*unpitched_voices*/) const {
  set_xml_string(node, "voice_name", voice_name.toStdString());
  note_fields_to_xml(node);
}
