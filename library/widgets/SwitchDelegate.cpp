#include "widgets/SwitchDelegate.hpp"

#include <QtWidgets/QSpinBox>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "cell_editors/StringPicker.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "other/Song.hpp"

namespace {

auto create_string_picker(const SwitchDelegate& delegate,
                          QWidget* parent_pointer, const QList<QString>& names)
    -> StringPicker& {
  auto& specific_result = get_reference(
      new StringPicker(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, names));
  specific_result.setFrame(false);
  // commit as soon as the user picks an item, instead of waiting for enter
  auto& mutable_delegate = const_cast<
      SwitchDelegate&>(  // NOLINT(cppcoreguidelines-pro-type-const-cast)
      delegate);
  QObject::connect(&specific_result, &QComboBox::activated, &mutable_delegate,
                   [&mutable_delegate, &specific_result]() {
                     emit mutable_delegate.commitData(&specific_result);
                   });
  return specific_result;
}

auto create_interval_editor(QWidget* parent_pointer) -> QWidget& {
  auto& editor = get_reference(
      new IntervalEditor(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
  editor.setFrameShape(QFrame::NoFrame);
  return editor;
}

auto create_rational_editor(QWidget* parent_pointer) -> QWidget& {
  auto& editor = get_reference(
      new RationalEditor(  // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
  editor.setFrameShape(QFrame::NoFrame);
  return editor;
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

auto create_column_editor(const SwitchDelegate& delegate,
                          QWidget* parent_pointer, const RowType row_type,
                          const int column) -> QWidget* {
  const auto& song = delegate.song;
  switch (row_type) {
    case RowType::chord_type:
      switch (static_cast<ChordColumn>(column)) {
        case ChordColumn::chord_interval_column:
          return &create_interval_editor(parent_pointer);
        case ChordColumn::chord_beats_column:
        case ChordColumn::chord_velocity_ratio_column:
        case ChordColumn::chord_tempo_ratio_column:
          return &create_rational_editor(parent_pointer);
        default:
          return nullptr;
      }
    case RowType::pitched_note_type:
      switch (static_cast<PitchedNoteColumn>(column)) {
        case PitchedNoteColumn::pitched_note_voice_name_column:
          return &create_string_picker(delegate, parent_pointer,
                                       get_names(song.pitched_voices));
        case PitchedNoteColumn::pitched_note_interval_column:
          return &create_interval_editor(parent_pointer);
        case PitchedNoteColumn::pitched_note_beats_column:
        case PitchedNoteColumn::pitched_note_velocity_ratio_column:
          return &create_rational_editor(parent_pointer);
        default:
          return nullptr;
      }
    case RowType::unpitched_note_type:
      switch (static_cast<UnpitchedNoteColumn>(column)) {
        case UnpitchedNoteColumn::unpitched_note_voice_name_column:
          return &create_string_picker(delegate, parent_pointer,
                                       get_names(song.unpitched_voices));
        case UnpitchedNoteColumn::unpitched_note_beats_column:
        case UnpitchedNoteColumn::unpitched_note_velocity_ratio_column:
          return &create_rational_editor(parent_pointer);
        default:
          return nullptr;
      }
    case RowType::pitched_voice_type:
      switch (static_cast<PitchedVoiceColumn>(column)) {
        case PitchedVoiceColumn::pitched_voice_instrument_column:
          return &create_string_picker(delegate, parent_pointer,
                                       get_some_program_names(true));
        case PitchedVoiceColumn::pitched_voice_velocity_ratio_column:
          return &create_rational_editor(parent_pointer);
        default:
          return nullptr;
      }
    case RowType::unpitched_voice_type:
      switch (static_cast<UnpitchedVoiceColumn>(column)) {
        case UnpitchedVoiceColumn::unpitched_voice_percussion_set_column:
          return &create_string_picker(delegate, parent_pointer,
                                       get_some_program_names(false));
        case UnpitchedVoiceColumn::unpitched_voice_midi_number_column:
          return &create_midi_number_editor(parent_pointer);
        case UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column:
          return &create_rational_editor(parent_pointer);
        default:
          return nullptr;
      }
  }
  Q_UNREACHABLE();
}

}  // namespace

auto create_switch_editor(const SwitchDelegate& delegate,
                          QWidget* parent_pointer, const RowType row_type,
                          const int column) -> QWidget* {
  auto* const result_pointer =
      create_column_editor(delegate, parent_pointer, row_type, column);
  if (result_pointer != nullptr) {
    auto& result = get_reference(result_pointer);
    result.setSizePolicy(QSizePolicy::Ignored,
                         result.sizePolicy().verticalPolicy());
  }
  return result_pointer;
}

auto SwitchDelegate::createEditor(QWidget* parent_pointer,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const -> QWidget* {
  auto* const result_pointer = create_switch_editor(
      *this, parent_pointer, current_row_type, index.column());
  if (result_pointer != nullptr) {
    return result_pointer;
  }
  return QStyledItemDelegate::createEditor(parent_pointer, option, index);
}
