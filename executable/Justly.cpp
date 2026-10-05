#include <QtWidgets/QApplication>
// #include "SDL.h"

#include "widgets/MainWindow.hpp"
#include "widgets/SongFile.hpp"
#include "widgets/WindowBody.hpp"

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
  if (maybe_restore_recovery(main_window.window_body)) {
    song_reloaded(main_window);
  }
  return QApplication::exec();
}
