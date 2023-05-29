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
  QString default_orchestra_text = R""""(
nchnls = 2
0dbfs = 1
instr BandedWG
    a_oscilator STKBandedWG p4, p5
    outs a_oscilator, a_oscilator
endin
instr BeeThree
    a_oscilator STKBeeThree p4, p5
    outs a_oscilator, a_oscilator
endin
instr BlowBotl
    a_oscilator STKBlowBotl p4, p5
    outs a_oscilator, a_oscilator
endin
instr BlowHole
    a_oscilator STKBlowHole p4, p5
    outs a_oscilator, a_oscilator
endin
instr Bowed
    a_oscilator STKBowed p4, p5
    outs a_oscilator, a_oscilator
endin
instr Brass
    a_oscilator STKBrass p4, p5
    outs a_oscilator, a_oscilator
endin
instr Clarinet
    a_oscilator STKClarinet p4, p5
    outs a_oscilator, a_oscilator
endin
instr Drummer
    a_oscilator STKDrummer p4, p5
    outs a_oscilator, a_oscilator
endin
instr FMVoices
    a_oscilator STKFMVoices p4, p5
    outs a_oscilator, a_oscilator
endin
instr Flute
    a_oscilator STKFlute p4, p5
    outs a_oscilator, a_oscilator
endin
instr HevyMetl
    a_oscilator STKHevyMetl p4, p5
    outs a_oscilator, a_oscilator
endin
instr Mandolin
    a_oscilator STKMandolin p4, p5
    outs a_oscilator, a_oscilator
endin
instr ModalBar
    a_oscilator STKModalBar p4, p5
    outs a_oscilator, a_oscilator
endin
instr Moog
    a_oscilator STKMoog p4, p5
    outs a_oscilator, a_oscilator
endin
instr PercFlut
    a_oscilator STKPercFlut p4, p5
    outs a_oscilator, a_oscilator
endin
instr Plucked
    a_oscilator STKPlucked p4, p5
    outs a_oscilator, a_oscilator
endin
instr Resonate
    a_oscilator STKResonate p4, p5
    outs a_oscilator, a_oscilator
endin
instr Rhodey
    a_oscilator STKRhodey p4, p5
    outs a_oscilator, a_oscilator
endin
instr Saxofony
    a_oscilator STKSaxofony p4, p5
    outs a_oscilator, a_oscilator
endin
instr Shakers
    a_oscilator STKShakers p4, p5
    outs a_oscilator, a_oscilator
endin
instr Simple
    a_oscilator STKSimple p4, p5
    outs a_oscilator, a_oscilator
endin
instr Sitar
    a_oscilator STKSitar p4, p5
    outs a_oscilator, a_oscilator
endin
instr StifKarp
    a_oscilator STKStifKarp p4, p5
    outs a_oscilator, a_oscilator
endin
instr TubeBell
    a_oscilator STKTubeBell p4, p5
    outs a_oscilator, a_oscilator
endin
instr VoicForm
    a_oscilator STKVoicForm p4, p5
    outs a_oscilator, a_oscilator
endin
instr Whistle
    a_oscilator STKWhistle p4, p5
    outs a_oscilator, a_oscilator
endin
instr Wurley
    a_oscilator STKWurley p4, p5
    outs a_oscilator, a_oscilator
endin
)"""";
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
