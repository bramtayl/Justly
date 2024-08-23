#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QtGlobal>

#include "justly/justly.hpp"

class SongEditor;

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
  SongEditor* song_editor_pointer = make_song_editor();
  show_song_editor(song_editor_pointer);
  QApplication::exec();
  delete_song_editor(song_editor_pointer);
}
