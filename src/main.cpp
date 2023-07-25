#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication

#include "Editor.h"  // for Editor
#include "Player.h"
#include "Song.h"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  
  QGuiApplication::setApplicationDisplayName("Justly");
  Song song;
  Player player(song, "devaudio", qEnvironmentVariable("AUDIO_DRIVER"));
  Editor editor(song, player);
  editor.show();
  QApplication::exec();
  return 0;
}
