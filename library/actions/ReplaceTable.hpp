#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QFlags>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtAssert>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QAction>
#include <QtGui/QUndoStack>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableView>
#include <algorithm>

#include "actions/ChangeId.hpp"
#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "cell_editors/StringPicker.hpp"
#include "cell_types/Program.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/EditMenu.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/PitchedVoicesModel.hpp"
#include "models/RowsModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "models/UnpitchedVoicesModel.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/RowType.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/piano_roll/PianoRollWidget.hpp"

[[nodiscard]] auto get_string_picker_width(const QList<QString> &names) -> int;

void set_minimum_column_size(QTableView &view, const int column_number,
                             const int minimum_size);

[[nodiscard]] auto get_is_voice(const RowType row_type) -> bool;

// applies a selection directly to piano_roll_widget's own fields: highlight
// the corresponding note bar(s), jump the cursor to the selection's start,
// and scroll to keep both in view. number_of_rows == 0 clears the highlight
// and hides the cursor (used both for "nothing selected" and for voice-row
// selections, which have no timeline position). Only called from
// update_piano_roll_selection() below, which derives these arguments from
// the switch table's own current selection.
void update_piano_roll_widget_selection(PianoRollWidget &widget,
                                        const RowType row_type,
                                        const int chord_number,
                                        const int first_row_number,
                                        const int number_of_rows);

// mirrors the switch table's current selection onto the piano roll (which
// note bar(s) get highlighted, where the cursor jumps to); an empty
// selection clears both, since get_only_range() asserts on an empty range
void update_piano_roll_selection(PianoRollWidget &piano_roll_widget,
                                 const SongWidget &song_widget);

void update_actions(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                    const QItemSelectionModel &selector);

void replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                   const RowType new_row_type,
                   const int new_chord_number,
                   PianoRollWidget &piano_roll_widget,
                   const int new_note_number = -1);

struct ReplaceTable : public QUndoCommand {
  SongMenuBar &song_menu_bar;
  SongWidget &song_widget;
  PianoRollWidget &piano_roll_widget;
  const RowType old_row_type;
  const int old_chord_number;
  RowType new_row_type;
  int new_chord_number;
  int new_note_number;

  explicit ReplaceTable(SongMenuBar &song_menu_bar_input,
                        SongWidget &song_widget_input,
                        PianoRollWidget &piano_roll_widget_input,
                        const RowType new_row_type_input,
                        const int new_chord_number_input,
                        const int new_note_number_input = -1)
      : song_menu_bar(song_menu_bar_input), song_widget(song_widget_input),
        piano_roll_widget(piano_roll_widget_input),
        old_row_type(
            song_widget.switch_column.switch_table.delegate.current_row_type),
        old_chord_number(
            get_parent_chord_number(song_widget.switch_column.switch_table)),
        new_row_type(new_row_type_input),
        new_chord_number(new_chord_number_input),
        new_note_number(new_note_number_input){};

  [[nodiscard]] auto id() const -> int override;

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override;

  void undo() override;

  void redo() override;
};
