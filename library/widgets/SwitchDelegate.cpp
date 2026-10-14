#include "widgets/SwitchDelegate.hpp"

#include <QtWidgets/QSpinBox>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "cell_editors/StringPicker.hpp"
#include "cell_editors/TextEditor.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "song/Song.hpp"

namespace {

auto create_string_picker(const SwitchDelegate& delegate,
                          QWidget* parent_pointer, const QList<QString>& names)
    -> StringPicker& {
  auto& specific_result = get_reference(
      new StringPicker(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, names));
  specific_result.setFrame(false);
  // commit as soon as the user picks an item, instead of waiting for enter
  auto& mutable_delegate =
      const_cast<  // NOLINT(cppcoreguidelines-pro-type-const-cast)
          SwitchDelegate&>(delegate);
  QObject::connect(&specific_result, &QComboBox::activated, &mutable_delegate,
                   [&mutable_delegate, &specific_result]() {
                     emit mutable_delegate.commitData(&specific_result);
                   });
  return specific_result;
}

auto create_interval_editor(QWidget* parent_pointer) -> QWidget& {
  return get_reference(
      new IntervalEditor(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
}

auto create_rational_editor(QWidget* parent_pointer) -> QWidget& {
  return get_reference(
      new RationalEditor(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
}

auto create_text_editor(QWidget* parent_pointer) -> QWidget& {
  return get_reference(
      new TextEditor(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
}

auto create_midi_number_editor(QWidget* parent_pointer) -> QWidget& {
  static const auto MAX_MIDI_NUMBER = 127;
  auto& editor =
      get_reference(new QSpinBox(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
  editor.setRange(0, MAX_MIDI_NUMBER);
  editor.setFrame(false);
  return editor;
}

}  // namespace

auto create_switch_editor(const SwitchDelegate& delegate,
                          QWidget* parent_pointer, const RowType row_type,
                          const int column) -> QWidget& {
  const auto& song = delegate.song;
  switch (row_type) {
    case RowType::chord_type:
      switch (static_cast<ChordColumn>(column)) {
        case ChordColumn::interval_column:
          return create_interval_editor(parent_pointer);
        case ChordColumn::words_column:
          return create_text_editor(parent_pointer);
        case ChordColumn::beats_column:
        case ChordColumn::velocity_ratio_column:
        case ChordColumn::tempo_ratio_column:
          return create_rational_editor(parent_pointer);
        default:
          Q_UNREACHABLE();
      }
    case RowType::pitched_note_type:
      switch (static_cast<PitchedNoteColumn>(column)) {
        case PitchedNoteColumn::voice_name_column:
          return create_string_picker(delegate, parent_pointer,
                                       get_names(song.pitched_voices));
        case PitchedNoteColumn::interval_column:
          return create_interval_editor(parent_pointer);
        case PitchedNoteColumn::beats_column:
        case PitchedNoteColumn::velocity_ratio_column:
          return create_rational_editor(parent_pointer);
        case PitchedNoteColumn::words_column:
          return create_text_editor(parent_pointer);
        default:
          Q_UNREACHABLE();
      }
    case RowType::unpitched_note_type:
      switch (static_cast<UnpitchedNoteColumn>(column)) {
        case UnpitchedNoteColumn::unpitched_note_voice_name_column:
          return create_string_picker(delegate, parent_pointer,
                                       get_names(song.unpitched_voices));
        case UnpitchedNoteColumn::unpitched_note_beats_column:
        case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
          return create_rational_editor(parent_pointer);
        case UnpitchedNoteColumn::unpitched_note_words_column:
          return create_text_editor(parent_pointer);
        default:
          Q_UNREACHABLE();
      }
    case RowType::pitched_voice_type:
      switch (static_cast<PitchedVoiceColumn>(column)) {
        case PitchedVoiceColumn::name_column:
          return create_text_editor(parent_pointer);
        case PitchedVoiceColumn::instrument_column:
          return create_string_picker(delegate, parent_pointer,
                                       get_some_program_names(true));
        case PitchedVoiceColumn::velocity_ratio_column:
          return create_rational_editor(parent_pointer);
        default:
          Q_UNREACHABLE();
      }
    case RowType::unpitched_voice_type:
      switch (static_cast<UnpitchedVoiceColumn>(column)) {
        case UnpitchedVoiceColumn::name_column:
          return create_text_editor(parent_pointer);
        case UnpitchedVoiceColumn::percussion_set_column:
          return create_string_picker(delegate, parent_pointer,
                                       get_some_program_names(false));
        case UnpitchedVoiceColumn::midi_number_column:
          return create_midi_number_editor(parent_pointer);
        case UnpitchedVoiceColumn::velocity_ratio_column:
          return create_rational_editor(parent_pointer);
        default:
          Q_UNREACHABLE();
      }
  }
  Q_UNREACHABLE();
}

auto SwitchDelegate::createEditor(QWidget* parent_pointer,
                                  const QStyleOptionViewItem& /*option*/,
                                  const QModelIndex& index) const -> QWidget* {
  return &create_switch_editor(*this, parent_pointer, current_row_type,
                               index.column());
}
