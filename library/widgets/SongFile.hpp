#pragma once

#include <QtCore/QString>

struct WindowBody;

[[nodiscard]] auto can_discard_changes(WindowBody& window_body) -> bool;

// recovery.xml's presence means the app didn't reach a clean shutdown last
// time (see connect_recovery_timer and MainWindow::closeEvent); its content
// mirrors save_as_file's format so it can be reloaded via open_file
[[nodiscard]] auto get_recovery_file_path() -> QString;

void remove_recovery_file();

void write_recovery_file(WindowBody& window_body);

void save_as_file(WindowBody& window_body, const QString& filename);

[[nodiscard]] auto open_file(WindowBody& window_body, const QString& filename)
    -> bool;

// call after MainWindow is constructed and shown: recovery.xml only exists
// if the previous session didn't reach a clean shutdown (see
// connect_recovery_timer and MainWindow::closeEvent). Returns whether a
// recovery was actually loaded, so callers know whether to refresh
[[nodiscard]] auto maybe_restore_recovery(WindowBody& window_body) -> bool;

void connect_recovery_timer(WindowBody& window_body);

[[nodiscard]] auto import_musicxml(WindowBody& window_body,
                                   const QString& filename) -> bool;
