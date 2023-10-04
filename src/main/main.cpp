#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication

#include "main/Editor.h"  // for Editor

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);

  QGuiApplication::setApplicationDisplayName("Justly");
  Editor editor;
  editor.show();
  QApplication::exec();
}
