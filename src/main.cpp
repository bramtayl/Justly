#include <QtCore/qglobal.h>   // for qCritical
#include <qapplication.h>     // for QApplication
#include <qguiapplication.h>  // for QGuiApplication
#include <qstring.h>          // for QString

#include "Editor.h"  // for Editor
#include "Song.h"    // for Song

auto get_file_text(const QString &filename) -> QString {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    QByteArray raw_string = filename.toLocal8Bit();
    qCritical("File %s doesn't exist",
              raw_string.data());
  }
  return QTextStream(&file).readAll();
}

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  QString default_orchestra_text = get_file_text("/home/brandon/Justly/src/orchestra.orc");
  QString default_instrument = "Plucked";
  Editor editor(default_orchestra_text, default_instrument);
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
