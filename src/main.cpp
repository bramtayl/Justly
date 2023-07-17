#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication

#include "Editor.h"  // for Editor
#include "Player.h"
#include "Song.h"

// PortAudio by default
// You can change to e.g. "pulse"
const auto AUDIO_DRIVER = "";

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  
  QGuiApplication::setApplicationDisplayName("Justly");
  Song song;
  Player player(song, "devaudio", AUDIO_DRIVER);
  Editor editor(song, player);
  editor.show();
  QApplication::exec();
  return 0;
}
