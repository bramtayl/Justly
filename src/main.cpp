#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication

#include "Editor.h"  // for Editor
#include "Player.h"
#include "Song.h"
#include "utilities.h"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  
  QGuiApplication::setApplicationDisplayName("Justly");
  // for the build executable, the executable folder is the config folde
  // the parent is the build folder
  // the parent of that is the source directory
  auto soundfont_file = find_soundfont_file();
  if (soundfont_file.isNull()) {
    return 0;
  }
  Song song(soundfont_file);
  Player player(song, "devaudio");
  Editor editor(song, player);
  editor.show();
  QApplication::exec();
}
