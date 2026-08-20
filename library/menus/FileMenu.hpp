#pragma once

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

struct SongWidget;

[[nodiscard]] auto make_file_dialog(SongWidget& song_widget,
                                    const char* caption, const QString& filter,
                                    QFileDialog::AcceptMode accept_mode,
                                    const QString& suffix,
                                    QFileDialog::FileMode file_mode)
    -> QFileDialog&;

[[nodiscard]] auto get_selected_file(SongWidget& song_widget,
                                     const QFileDialog& dialog) -> QString;

struct FileMenu : public QMenu {
  QAction save_action;
  QAction open_action;
  QAction save_as_action;
  QAction import_action;
  QAction export_action;
  QAction export_midi_action;

  explicit FileMenu(SongWidget& song_widget);
};
