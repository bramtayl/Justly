#include <QtCore/qglobal.h>   // for qCritical
#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication
#include <qstring.h>          // for QString

#include "Editor.h"  // for Editor
#include "Song.h"    // for Song

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);

  Editor editor;
  QString file;
  if (number_of_arguments == 1) {
    file = Editor::choose_file();
  } else if (number_of_arguments == 2) {
    file = arguments[1];
  } else {
    qCritical("Wrong number of arguments %d!", number_of_arguments);
  }
  if (file != "") {
    editor.load(file);
    QGuiApplication::setApplicationDisplayName(file);

    editor.show();
    QApplication::exec();
    editor.song.save(file);
  }
  return 0;
}
