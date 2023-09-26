#include <qapplication.h>  // for QApplication
#include <qguiapplication.h>   // for QGuiApplication

#include "main/Editor.h"  // for Editor
#include "main/Song.h"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);

  QGuiApplication::setApplicationDisplayName("Justly");
  Song song;
  Editor editor(&song);
  editor.show();
  QApplication::exec();
}
