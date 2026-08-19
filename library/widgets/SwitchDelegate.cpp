#include "widgets/SwitchDelegate.hpp"

auto create_string_picker(QWidget *parent_pointer,
                          const QList<QString> &names) -> StringPicker & {
  auto &specific_result =
      get_reference(new StringPicker( // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, names));
  specific_result.setFrame(false);
  return specific_result;
}

auto SwitchDelegate::createEditor(QWidget *parent_pointer, const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const -> QWidget * {
  const auto column = index.column();
  QWidget *result_pointer = nullptr;
  if ((current_row_type == RowType::chord_type && column == static_cast<int>(ChordColumn::chord_interval_column)) ||
      ((current_row_type == RowType::pitched_note_type) &&
       (column == static_cast<int>(PitchedNoteColumn::pitched_note_interval_column)))) {
    auto &specific_result = get_reference(
        new IntervalEditor( // NOLINT(cppcoreguidelines-owning-memory)
            parent_pointer));
    specific_result.setFrameShape(QFrame::NoFrame);
    result_pointer = &specific_result;
  }
  if ((current_row_type == RowType::chord_type &&
       (column == static_cast<int>(ChordColumn::chord_beats_column) ||
        column == static_cast<int>(ChordColumn::chord_velocity_ratio_column) ||
        column == static_cast<int>(ChordColumn::chord_tempo_ratio_column))) ||
      ((current_row_type == RowType::pitched_note_type) &&
       (column == static_cast<int>(PitchedNoteColumn::pitched_note_beats_column) ||
        column == static_cast<int>(PitchedNoteColumn::pitched_note_velocity_ratio_column))) ||
      ((current_row_type == RowType::unpitched_note_type) &&
       (column == static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column) ||
        column == static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column))) ||
      (current_row_type == RowType::pitched_voice_type &&
       column == static_cast<int>(PitchedVoiceColumn::pitched_voice_velocity_ratio_column)) ||
      (current_row_type == RowType::unpitched_voice_type &&
       column == static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column))) {
    auto &specific_result = get_reference(
        new RationalEditor( // NOLINT(cppcoreguidelines-owning-memory)
            parent_pointer));
    specific_result.setFrameShape(QFrame::NoFrame);
    result_pointer = &specific_result;
  }
  if (current_row_type == RowType::unpitched_voice_type &&
      column == static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column)) {
    auto &specific_result = get_reference(
        new MidiNumberEditor( // NOLINT(cppcoreguidelines-owning-memory)
            parent_pointer));
    specific_result.setFrame(false);
    result_pointer = &specific_result;
  }
  if (current_row_type == RowType::pitched_note_type &&
       column == static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column)) {
    result_pointer =
        &create_voice_number_picker(parent_pointer, song.pitched_voices);
  }
  if (current_row_type == RowType::unpitched_note_type &&
       column == static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column)) {
    result_pointer =
        &create_voice_number_picker(parent_pointer, song.unpitched_voices);
  }
  if (current_row_type == RowType::pitched_voice_type &&
      column == static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column)) {
    result_pointer =
        &create_string_picker(parent_pointer, get_some_program_names(true));
  }
  if (current_row_type == RowType::unpitched_voice_type &&
      column == static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column)) {
    result_pointer =
        &create_string_picker(parent_pointer, get_some_program_names(false));
  }
  if (result_pointer != nullptr) {
    auto &result = get_reference(result_pointer);
    result.setSizePolicy(QSizePolicy::Ignored,
                         result.sizePolicy().verticalPolicy());
    return result_pointer;
  }
  return QStyledItemDelegate::createEditor(parent_pointer, option, index);
}
