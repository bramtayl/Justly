#include "rows/UnpitchedNote.hpp"

#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "rows/UnpitchedVoice.hpp"

void UnpitchedNote::from_xml(xmlNode& node) {
  auto* field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto& field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "beats") {
      set_rational_from_xml(beats, field_node);
    } else if (name == "velocity_ratio") {
      set_rational_from_xml(velocity_ratio, field_node);
    } else if (name == "words") {
      words = get_qstring_content(field_node);
    } else if (name == "voice_number") {
      voice_number = xml_to_int(field_node);
    } else {
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
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
    case UnpitchedNoteColumn::unpitched_note_voice_number_column:
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

auto UnpitchedNote::is_pitched() -> bool { return false; }

auto UnpitchedNote::get_closest_midi(
    QWidget& /*parent*/, Player& /*player*/,
    const QList<UnpitchedVoice>& unpitched_voices, const int /*channel_number*/,
    int /*chord_number*/, int /*note_number*/) const -> std::optional<short> {
  return unpitched_voices.at(voice_number).midi_number;
}

auto UnpitchedNote::get_program(
    const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& unpitched_voices) const -> const Program& {
  return get_voice_program(get_some_programs(false), unpitched_voices,
                           voice_number);
}

auto UnpitchedNote::get_voice_velocity_ratio(
    const QList<PitchedVoice>& /*pitched_voices*/,
    const QList<UnpitchedVoice>& unpitched_voices) const -> const Rational& {
  return unpitched_voices.at(voice_number).velocity_ratio;
}

auto UnpitchedNote::get_data(const int column_number) const -> QVariant {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_number_column:
      return voice_number;
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
    case UnpitchedNoteColumn::unpitched_note_voice_number_column:
      voice_number = variant_to<int>(new_value);
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
    case UnpitchedNoteColumn::unpitched_note_voice_number_column:
      voice_number = template_row.voice_number;
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

void UnpitchedNote::column_to_xml(xmlNode& node,
                                  const int column_number) const {
  switch (static_cast<UnpitchedNoteColumn>(column_number)) {
    case UnpitchedNoteColumn::number_of_unpitched_note_columns:
      Q_UNREACHABLE();
    case UnpitchedNoteColumn::unpitched_note_voice_number_column:
      set_xml_int(node, "voice_number", voice_number);
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

void UnpitchedNote::to_xml(xmlNode& node) const {
  set_xml_int(node, "voice_number", voice_number);
  maybe_add_rational_to_xml(node, "beats", beats);
  maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
  maybe_add_qstring_to_xml(node, "words", words);
}
