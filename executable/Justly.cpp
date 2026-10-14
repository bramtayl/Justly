#include <QtWidgets/QApplication>

#include "io/FileOpener.hpp"
#include "io/song_file.hpp"
#include "widgets/MainWindow.hpp"

auto main(int number_of_arguments, char* arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  // some Linux platform theme plugins (e.g. GTK3) call setlocale(LC_ALL, "")
  // during QApplication construction, which can switch LC_NUMERIC to a
  // locale using a comma decimal separator; force it back to "C" so any
  // remaining locale-sensitive C calls stay consistent
  static_cast<void>(std::setlocale(  // NOLINT(concurrency-mt-unsafe)
      LC_NUMERIC, "C"));

  set_up();
  MainWindow main_window;
  main_window.show();

  // a file to open, e.g. from double-clicking a song, comes on the command
  // line on Linux and Windows, or in a file open event on macOS
  FileOpener file_opener(main_window);
  QApplication::instance()->installEventFilter(&file_opener);
  const auto command_line = QApplication::arguments();
  if (command_line.size() > 1) {
    file_opener.open(command_line.at(1));
  }

  maybe_restore_recovery(main_window);
  file_opener.start();
  return QApplication::exec();
}
