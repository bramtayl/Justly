#include <QtWidgets/QApplication>
// #include "SDL.h"

#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"

auto main(int number_of_arguments, char* arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  // some Linux platform theme plugins (e.g. GTK3) call setlocale(LC_ALL, "")
  // during QApplication construction, which can switch LC_NUMERIC to a
  // locale using a comma decimal separator; force it back to "C" so any
  // remaining locale-sensitive C calls stay consistent
  static_cast<void>(std::setlocale(  // NOLINT(concurrency-mt-unsafe)
      LC_NUMERIC, "C"));

  set_up();
  SongEditor song_editor;
  song_editor.show();
  if (maybe_restore_recovery(song_editor.song_widget)) {
    song_reloaded(song_editor.song_menu_bar, song_editor.song_widget,
                  song_editor.piano_roll_widget);
  }
  return QApplication::exec();
}
