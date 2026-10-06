#pragma once

#include <QtCore/QString>

struct MainWindow;
struct WindowBody;

[[nodiscard]] auto can_discard_changes(WindowBody& window_body) -> bool;

// recovery.xml's presence means the app didn't reach a clean shutdown last
// time (see connect_recovery_timer and MainWindow::closeEvent); its content
// mirrors save_as_file's format so it can be reloaded via open_file
[[nodiscard]] auto get_recovery_file_path() -> QString;

void remove_recovery_file();

void write_recovery_file(WindowBody& window_body);

void save_as_file(WindowBody& window_body, const QString& filename);

// open_file, import_musicxml, and maybe_restore_recovery replace the song
// wholesale, bypassing the undo stack, then reset the view back to chords
// and rebuild the piano roll themselves. Each returns whether the song was
// actually replaced; a rejected file leaves the song and view untouched

auto open_file(MainWindow& main_window, const QString& filename) -> bool;

auto import_musicxml(MainWindow& main_window, const QString& filename) -> bool;

// call after MainWindow is constructed and shown: recovery.xml only exists
// if the previous session didn't reach a clean shutdown (see
// connect_recovery_timer and MainWindow::closeEvent)
auto maybe_restore_recovery(MainWindow& main_window) -> bool;

void connect_recovery_timer(WindowBody& window_body);
