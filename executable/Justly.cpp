#include <QtWidgets/QApplication>
// #include "SDL.h"

#include "widgets/SongEditor.hpp"

auto main(int number_of_arguments, char *arguments[]) -> int {
  // SDL_Init(SDL_INIT_AUDIO);
  QApplication const app(number_of_arguments, arguments);

  set_up();
  SongEditor song_editor;
  song_editor.show();
  QApplication::exec();
}
