#pragma once

#include <QtWidgets/QComboBox>
#include <QtWidgets/QStyledItemDelegate>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/MidiNumberEditor.hpp"
#include "cell_editors/ProgramEditor.hpp"
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

struct SwitchDelegate : public QStyledItemDelegate {
  Song &song;
  RowType current_row_type = chord_type;
  QStringListModel string_model;

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
    if ((current_row_type == chord_type &&
         column == pitched_note_voice_column) ||
        ((current_row_type == pitched_note_type) &&
         (column == pitched_note_voice_column))) {
      result_pointer =
          &create_string_picker(parent_pointer, get_names(song.pitched_voices));
    }
    if ((current_row_type == chord_type &&
         column == unpitched_note_voice_column) ||
        ((current_row_type == unpitched_note_type) &&
         (column == unpitched_note_voice_column))) {
      result_pointer = &create_string_picker(parent_pointer,
                                             get_names(song.unpitched_voices));
    }
    if (current_row_type == pitched_voice_type &&
        column == pitched_voice_instrument_column) {
      auto &specific_result = get_reference(
          new ProgramEditor( // NOLINT(cppcoreguidelines-owning-memory)
              parent_pointer, true));
      specific_result.setFrame(false);
      result_pointer = &specific_result;
    }
    if (current_row_type == unpitched_voice_type &&
        column == unpitched_voice_percussion_set_column) {
      auto &specific_result = get_reference(
          new ProgramEditor( // NOLINT(cppcoreguidelines-owning-memory)
              parent_pointer, false));
      specific_result.setFrame(false);
      result_pointer = &specific_result;
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