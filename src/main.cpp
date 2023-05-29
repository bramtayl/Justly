#include <QtCore/qglobal.h>   // for qCritical
#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication
#include <qstring.h>          // for QString

#include "Editor.h"  // for Editor
#include "Song.h"    // for Song

auto get_file_text(const QString &filename) -> QString {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qCritical("File %s doesn't exist",
              filename.toStdString().c_str());
  }
  return QTextStream(&file).readAll();
}

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  QString default_orchestra_text = get_file_text("/home/brandon/Justly/src/orchestra.orc");
  Editor editor(default_orchestra_text, "Plucked");
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
