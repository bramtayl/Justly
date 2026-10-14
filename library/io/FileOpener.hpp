#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <optional>

struct MainWindow;

// opens a file the system asks Justly to, from the command line on Linux and
// Windows, or a file open event on macOS, e.g. from double-clicking a song.
// Holds onto it until start, so it doesn't open while the recovery prompt
// is up, since macOS can send the event during that prompt's event loop
struct FileOpener : public QObject {
  MainWindow& main_window;
  bool started = false;
  std::optional<QString> pending_file;

  explicit FileOpener(MainWindow& main_window_input);

  // opens filename now if started, or else when started
  void open(const QString& filename);

  void start();

  // takes file open events sent to the application
  auto eventFilter(QObject* watched, QEvent* event) -> bool override;
};
