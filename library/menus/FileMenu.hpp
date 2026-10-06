#pragma once

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMenu>

struct WindowBody;

// runs a file dialog starting in, and then remembering, the current folder;
// nullopt if the user cancels. Open dialogs pick an existing file, save
// dialogs any file
[[nodiscard]] auto maybe_choose_file(
    WindowBody& window_body, const QString& caption, const QString& filter,
    QFileDialog::AcceptMode accept_mode, const QString& suffix,
    const QString& accept_label = {}) -> std::optional<QString>;

struct FileMenu : public QMenu {
  QAction save_action;
  QAction open_action;
  QAction save_as_action;
  QAction import_action;
  QAction export_action;

  explicit FileMenu(WindowBody& window_body);
};
