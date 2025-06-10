#pragma once

#include <QtCore/QItemSelectionModel>
#include <QtGui/QAction>
#include <QtWidgets/QMenuBar>

#include "ControlsColumn.hpp"
#include "EditMenu.hpp"
#include "FileMenu.hpp"
#include "InsertMenu.hpp"
#include "IntervalRow.hpp"
#include "PasteMenu.hpp"
#include "PlayMenu.hpp"
#include "RowType.hpp"
#include "SongWidget.hpp"
#include "SwitchColumn.hpp"
#include "ViewMenu.hpp"

struct SongMenuBar : public QMenuBar {
  FileMenu file_menu;
  EditMenu edit_menu;
  ViewMenu view_menu;
  PlayMenu play_menu;

  explicit SongMenuBar(SongWidget &song_widget)
      : file_menu(FileMenu(song_widget)), edit_menu(EditMenu(song_widget)),
        play_menu(PlayMenu(song_widget)) {
    addMenu(&file_menu);
    addMenu(&edit_menu);
    addMenu(&view_menu);
    addMenu(&play_menu);
  }
};

static inline void update_actions(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                           const QItemSelectionModel &selector) {
  auto &edit_menu = song_menu_bar.edit_menu;
  auto &controls_column = song_widget.controls_column;

  const auto anything_selected = !selector.selection().empty();
  const auto any_pitched_selected =
      anything_selected &&
      song_widget.switch_column.current_row_type != unpitched_note_type;

  set_interval_rows_enabled(controls_column.third_row, controls_column.fifth_row,
                   controls_column.seventh_row, controls_column.octave_row,
                   any_pitched_selected);

  edit_menu.cut_action.setEnabled(anything_selected);
  edit_menu.copy_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_over_action.setEnabled(anything_selected);
  edit_menu.paste_menu.paste_after_action.setEnabled(anything_selected);
  edit_menu.insert_menu.insert_after_action.setEnabled(anything_selected);
  edit_menu.delete_cells_action.setEnabled(anything_selected);
  edit_menu.remove_rows_action.setEnabled(anything_selected);
  song_menu_bar.play_menu.play_action.setEnabled(anything_selected);
}
