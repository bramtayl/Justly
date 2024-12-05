#include <QtWidgets/QApplication>

#include "justly/justly.hpp"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);

  set_up();
  auto& song_editor = make_song_editor();
  show_song_editor(song_editor);
  QApplication::exec();
  delete_song_editor(song_editor);
}
