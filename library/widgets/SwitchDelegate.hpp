#pragma once

#include <QtWidgets/QComboBox>
#include <QtWidgets/QStyledItemDelegate>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/MidiNumberEditor.hpp"
#include "cell_editors/StringPicker.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "other/Song.hpp"
#include "rows/RowType.hpp"

static auto create_string_picker(QWidget *parent_pointer,
                                 const QList<QString> &names) -> auto & {
  auto &specific_result =
      get_reference(new StringPicker( // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, names));
  specific_result.setFrame(false);
  return specific_result;
}

static auto create_voice_number_picker(QWidget *parent_pointer, int number_of_voices) -> auto & {
  auto &specific_result =
      get_reference(new QSpinBox( // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer));
  specific_result.setFrame(false);
  specific_result.setMinimum(0);
  specific_result.setMaximum(number_of_voices);
  return specific_result;
}

struct SwitchDelegate : public QStyledItemDelegate {
  Song &song;
  RowType current_row_type = chord_type;

  explicit SwitchDelegate(Song &song_input, QWidget *parent)
      : QStyledItemDelegate(parent), song(song_input) {}

  auto createEditor(QWidget *parent_pointer, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const -> QWidget * override {
    const auto column = index.column();
    QWidget *result_pointer = nullptr;
    if ((current_row_type == chord_type && column == chord_interval_column) ||
        ((current_row_type == pitched_note_type) &&
         (column == pitched_note_interval_column))) {
      auto &specific_result = get_reference(
          new IntervalEditor( // NOLINT(cppcoreguidelines-owning-memory)
              parent_pointer));
      specific_result.setFrameShape(QFrame::NoFrame);
      result_pointer = &specific_result;
    }
    if ((current_row_type == chord_type &&
         (column == chord_beats_column ||
          column == chord_velocity_ratio_column ||
          column == chord_tempo_ratio_column)) ||
        ((current_row_type == pitched_note_type) &&
         (column == pitched_note_beats_column ||
          column == pitched_note_velocity_ratio_column)) ||
        ((current_row_type == unpitched_note_type) &&
         (column == unpitched_note_beats_column ||
          column == unpitched_note_velocity_ratio_column))) {
      auto &specific_result = get_reference(
          new RationalEditor( // NOLINT(cppcoreguidelines-owning-memory)
              parent_pointer));
      specific_result.setFrameShape(QFrame::NoFrame);
      result_pointer = &specific_result;
    }
    if (current_row_type == unpitched_voice_type &&
        column == unpitched_voice_midi_number_column) {
      auto &specific_result = get_reference(
          new MidiNumberEditor( // NOLINT(cppcoreguidelines-owning-memory)
              parent_pointer));
      specific_result.setFrame(false);
      result_pointer = &specific_result;
    }
    if (current_row_type == pitched_note_type &&
         column == pitched_note_voice_number_column) {
      result_pointer =
          &create_voice_number_picker(parent_pointer, static_cast<int>(song.pitched_voices.size() - 1));
    }
    if (current_row_type == unpitched_note_type &&
         column == unpitched_note_voice_number_column) {
      result_pointer = &create_voice_number_picker(parent_pointer,
                                             static_cast<int>(song.unpitched_voices.size() - 1));
    }
    if (current_row_type == pitched_voice_type &&
        column == pitched_voice_instrument_column) {
      result_pointer =
          &create_string_picker(parent_pointer, get_some_program_names(true));
    }
    if (current_row_type == unpitched_voice_type &&
        column == unpitched_voice_percussion_set_column) {
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
  };
};