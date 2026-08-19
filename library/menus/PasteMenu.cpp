#include "menus/PasteMenu.hpp"

#include <QtCore/QFlags>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtCore/qassert.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qobjectdefs.h>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>

#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/UnpitchedNote.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

auto get_mime_description(const QString &mime_type) -> QString {
  Q_ASSERT(mime_type.isValidUtf16());
  if (mime_type == Chord::get_cells_mime()) {
    return QObject::tr("chords cells");
  }
  if (mime_type == PitchedNote::get_cells_mime()) {
    return QObject::tr("pitched notes cells");
  }
  if (mime_type == UnpitchedNote::get_cells_mime()) {
    return QObject::tr("unpitched notes cells");
  }
  return mime_type;
}

static void add_paste_insert(SongWidget &song_widget, const int row_number) {
  auto &switch_column = song_widget.switch_column;
  auto &switch_table = switch_column.switch_table;

  auto *undo_command = dispatch_row_type(
      switch_table, [&switch_table, row_number](
                        auto &rows_model) -> QUndoCommand * {
        return make_paste_insert_command(switch_table, rows_model, row_number);
      });
  if (undo_command == nullptr) {
    return;
  }
  song_widget.undo_stack.push(undo_command);
}

PasteMenu::PasteMenu(SongWidget &song_widget) : QMenu(PasteMenu::tr("&Paste")) {
  add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
  add_menu_action(*this, paste_into_start_action);
  add_menu_action(*this, paste_after_action, QKeySequence::UnknownKey, false);
  paste_after_action.setShortcut(Qt::ControlModifier | Qt::ShiftModifier |
                                 Qt::Key_V);

  QObject::connect(
      &paste_over_action, &QAction::triggered, this, [&song_widget]() -> auto {
        auto &switch_table = song_widget.switch_column.switch_table;

        const auto first_row_number = get_only_range(switch_table).top();

        auto *undo_command = dispatch_row_type(
            switch_table, [&switch_table, first_row_number](
                              auto &rows_model) -> QUndoCommand * {
              return make_paste_cells_command(switch_table, first_row_number,
                                              rows_model);
            });
        if (undo_command == nullptr) {
          return;
        }
        song_widget.undo_stack.push(undo_command);
      });

  QObject::connect(&paste_into_start_action, &QAction::triggered, this,
                   [&song_widget]() -> auto { add_paste_insert(song_widget, 0); });

  QObject::connect(&paste_after_action, &QAction::triggered, this,
                   [&song_widget]() -> auto {
                     add_paste_insert(song_widget, get_next_row(song_widget));
                   });
}
