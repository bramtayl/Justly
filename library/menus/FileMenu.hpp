#pragma once

#include <QtCore/QString>
#include <QtGui/QAction>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

struct SongWidget;

[[nodiscard]] auto make_file_dialog(
    SongWidget &song_widget, const char * caption, const QString &filter,
     QFileDialog::AcceptMode accept_mode, const QString &suffix,
     QFileDialog::FileMode file_mode) -> QFileDialog &;

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
