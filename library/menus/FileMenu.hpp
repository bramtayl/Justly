#pragma once

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QtAssert>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"

[[nodiscard]] auto make_file_dialog(
    SongWidget &song_widget, const char *const caption, const QString &filter,
    const QFileDialog::AcceptMode accept_mode, const QString &suffix,
    const QFileDialog::FileMode file_mode) -> QFileDialog &;

[[nodiscard]] auto get_selected_file(SongWidget &song_widget,
                                     const QFileDialog &dialog) -> QString;

struct FileMenu : public QMenu {
  QAction save_action = QAction(FileMenu::tr("&Save"));
  QAction open_action = QAction(FileMenu::tr("&Open"));
  QAction save_as_action = QAction(FileMenu::tr("&Save As..."));
  QAction import_action = QAction(FileMenu::tr("&Import MusicXML"));
  QAction export_action = QAction(FileMenu::tr("&Export recording"));
  QAction export_midi_action = QAction(FileMenu::tr("Export &MIDI"));

  explicit FileMenu(SongWidget &song_widget);
};
