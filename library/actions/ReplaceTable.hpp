#pragma once

#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"

struct PianoRollWidget;
struct SongMenuBar;

void replace_table(SongMenuBar& song_menu_bar, SongWidget& song_widget,
                   RowType new_row_type, int new_chord_number,
                   PianoRollWidget& piano_roll_widget,
                   int new_note_number = -1);

struct ReplaceTable : public QUndoCommand {
  SongMenuBar& song_menu_bar;
  SongWidget& song_widget;
  PianoRollWidget& piano_roll_widget;
  const RowType old_row_type;
  const int old_chord_number;
  RowType new_row_type;
  int new_chord_number;
  int new_note_number;

  explicit ReplaceTable(SongMenuBar& song_menu_bar_input,
                        SongWidget& song_widget_input,
                        PianoRollWidget& piano_roll_widget_input,
                        const RowType new_row_type_input,
                        const int new_chord_number_input,
                        const int new_note_number_input = -1)
      : song_menu_bar(song_menu_bar_input),
        song_widget(song_widget_input),
        piano_roll_widget(piano_roll_widget_input),
        old_row_type(
            song_widget.switch_column.switch_table.delegate.current_row_type),
        old_chord_number(
            get_parent_chord_number(song_widget.switch_column.switch_table)),
        new_row_type(new_row_type_input),
        new_chord_number(new_chord_number_input),
        new_note_number(new_note_number_input) {};

  [[nodiscard]] auto id() const -> int override;

  [[nodiscard]] auto mergeWith(const QUndoCommand* next_command_pointer)
      -> bool override;

  void undo() override;

  void redo() override;
};
