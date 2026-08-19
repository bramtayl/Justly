#pragma once

#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/Qt>
#include <QtCore/QtAssert>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtWidgets/QMenu>
#include <algorithm>
#include <ranges>

#include "models/RowsModel.hpp"
#include "other/Song.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/RowType.hpp"
#include "sound/Player.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"

struct PlayState;

void modulate_before_chord(const Song &song, PlayState &play_state,
                           const int next_chord_number);

struct PlaySelection {
  RowType row_type;
  int chord_number; // -1 unless row_type is a note type
  int first_row_number;
  int number_of_rows;
};

[[nodiscard]] auto get_play_selection(const SongWidget &song_widget)
    -> PlaySelection;

struct PlayMenu : public QMenu {
  QAction play_action = QAction(PlayMenu::tr("&Play selection"));
  QAction play_to_end_action = QAction(PlayMenu::tr("Play to &end"));
  QAction stop_playing_action = QAction(PlayMenu::tr("&Stop playing"));

  explicit PlayMenu(SongWidget &song_widget);
};
