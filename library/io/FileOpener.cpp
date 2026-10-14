#include "io/FileOpener.hpp"

#include <QtGui/QFileOpenEvent>

#include "io/song_file.hpp"

FileOpener::FileOpener(MainWindow& main_window_input)
    : main_window(main_window_input) {}

void FileOpener::open(const QString& filename) {
  if (started) {
    open_or_import(main_window, filename);
  } else {
    // only one song can be open, so the last file wins
    pending_file = filename;
  }
}

void FileOpener::start() {
  started = true;
  if (pending_file.has_value()) {
    const auto filename = *pending_file;
    pending_file.reset();
    open_or_import(main_window, filename);
  }
}

auto FileOpener::eventFilter(QObject* watched, QEvent* event) -> bool {
  if (event->type() == QEvent::FileOpen) {
    open(static_cast<QFileOpenEvent*>(event)->file());
    return true;
  }
  return QObject::eventFilter(watched, event);
}
