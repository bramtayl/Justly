#include <QtWidgets/QApplication>

#include "justly/justly.hpp"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);

  set_up();
  SongEditor song_editor;
  song_editor.show();
  QApplication::exec();
}
