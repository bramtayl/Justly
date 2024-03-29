#include <qapplication.h>  // for QApplication

#include "justly/SongEditor.h"  // for SongEditor

auto main(int number_of_arguments, char *arguments[]) -> int {
  QApplication const app(number_of_arguments, arguments);
  QApplication::setApplicationDisplayName("Justly");
  SongEditor song_editor;
  song_editor.show();
  QApplication::exec();
}
