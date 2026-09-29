#pragma once

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

struct WindowBody;

[[nodiscard]] auto make_file_dialog(WindowBody& window_body,
                                    const char* caption, const QString& filter,
                                    QFileDialog::AcceptMode accept_mode,
                                    const QString& suffix,
                                    QFileDialog::FileMode file_mode)
    -> QFileDialog&;

[[nodiscard]] auto get_selected_file(WindowBody& window_body,
                                     const QFileDialog& dialog) -> QString;

struct FileMenu : public QMenu {
  QAction save_action;
  QAction open_action;
  QAction save_as_action;
  QAction import_action;
  QAction export_action;
  QAction export_midi_action;

  explicit FileMenu(WindowBody& window_body);
};
