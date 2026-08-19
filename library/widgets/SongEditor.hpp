#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaType>
#include <QtCore/QObject>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/Qt>
#include <QtGui/QAction>
#include <QtGui/QCloseEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>
#include <QtGui/QUndoStack>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <functional>
#include <libxml/xmlversion.h>
#include <optional>
#include <string>

#include "actions/ReplaceTable.hpp"
#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "menus/FileMenu.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/piano_roll/PianoRollWidget.hpp"
#include "widgets/piano_roll/PlayheadTransition.hpp"

// open_file/import_musicxml replace the song wholesale, bypassing the undo
// stack, so the usual indexChanged-driven refresh never fires for them --
// call this afterward. They also always land back on the chords view (see
// reset_switch_table_to_chords), so reuse replace_table to redo the same
// label/view-menu/selection reset it applies when switching there manually;
// unlike ordinary navigation it doesn't know the song's contents changed
// under it, so the piano roll scene still needs an explicit rebuild on top
void song_reloaded(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                   PianoRollWidget &piano_roll_widget);

void open_file_and_reload(SongMenuBar &song_menu_bar,
                          SongWidget &song_widget,
                          PianoRollWidget &piano_roll_widget,
                          const QString &filename);

void import_musicxml_and_reload(SongMenuBar &song_menu_bar,
                                SongWidget &song_widget,
                                PianoRollWidget &piano_roll_widget,
                                const QString &filename);

void zoom_in_piano_roll(PianoRollWidget &widget);

void zoom_out_piano_roll(PianoRollWidget &widget);

void stop_piano_roll_playhead(PianoRollWidget &widget);

void start_piano_roll_playhead(PianoRollWidget &widget,
                               const double baseline_ms,
                               const double end_ms);

struct SongEditor : public QMainWindow {
public:
  SongWidget &song_widget;
  SongMenuBar &song_menu_bar;
  PianoRollWidget &piano_roll_widget = *(new PianoRollWidget(song_widget));
  QDockWidget &piano_roll_dock =
      *(new QDockWidget(SongEditor::tr("Piano Roll"), this));

  explicit SongEditor();

  void closeEvent(QCloseEvent *close_event_pointer) override;
};

void set_up();
