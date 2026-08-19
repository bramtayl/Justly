#include "menus/InsertMenu.hpp"

#include "QtCore/qobjectdefs.h"
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <libxml/parser.h>

#include "actions/InsertRow.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/PitchedVoicesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "models/UnpitchedVoicesModel.hpp"
#include "other/Song.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedNote.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"

void add_insert_row(SongWidget &song_widget, const int row_number,
                    const RowType new_row_type) {
  auto &switch_table = song_widget.switch_column.switch_table;
  QUndoCommand *undo_command = nullptr;
  const auto &chords = song_widget.song.chords;
  switch (new_row_type) {
  case RowType::chord_type:
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, row_number);
    break;
  case RowType::pitched_note_type:
    undo_command =
        make_insert_note<PitchedVoice>(switch_table.pitched_notes_model, chords, row_number);
    break;
  case RowType::unpitched_note_type:
    undo_command = make_insert_note<UnpitchedVoice>(switch_table.unpitched_notes_model, chords,
                                    row_number);
    break;
  case RowType::pitched_voice_type:
    undo_command = make_insert_voice<PitchedVoice, PitchedNote>(
        switch_table.pitched_voices_model, row_number);
    break;
  case RowType::unpitched_voice_type:
    undo_command = make_insert_voice<UnpitchedVoice, UnpitchedNote>(
        switch_table.unpitched_voices_model, row_number);
    break;
  }
  song_widget.undo_stack.push(undo_command);
}

namespace {

void add_insert_row_default(SongWidget &song_widget, const int row_number) {
  add_insert_row(
      song_widget, row_number,
      song_widget.switch_column.switch_table.delegate.current_row_type);
}

}  // namespace

InsertMenu::InsertMenu(SongWidget &song_widget)
    : QMenu(InsertMenu::tr("&Insert row")) {
  add_menu_action(*this, insert_after_action,
                  QKeySequence::InsertLineSeparator, false);
  add_menu_action(*this, insert_into_start_action, QKeySequence::AddTab);

  QObject::connect(
      &insert_after_action, &QAction::triggered, this, [&song_widget]() -> auto {
        add_insert_row_default(song_widget, get_next_row(song_widget));
      });

  QObject::connect(
      &insert_into_start_action, &QAction::triggered, this,
      [&song_widget]() -> auto { add_insert_row_default(song_widget, 0); });
}
