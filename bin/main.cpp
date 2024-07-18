#include <QApplication>

#include "justly/SongEditor.hpp"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  QApplication::setApplicationDisplayName("Justly");
  register_converters();
  SongEditor song_editor;
  song_editor.show();
  QApplication::exec();
}
