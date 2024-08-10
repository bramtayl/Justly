#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QtGlobal>

#include "justly/SongEditor.hpp"

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  QApplication::setApplicationDisplayName("Justly");

  auto icon_file = QDir(QCoreApplication::applicationDirPath())
                       .filePath("../share/Justly.svg");
  Q_ASSERT(QFile::exists(icon_file));
  QIcon icon(icon_file);
  Q_ASSERT(!icon.isNull());
  QApplication::setWindowIcon(icon);

  register_converters();
  SongEditor song_editor;
  song_editor.show();
  QApplication::exec();
}
