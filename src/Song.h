#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qpointer.h>            // for QPointer
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT, signals
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "ChordsModel.h"
#include "TreeNode.h"   // for TreeNode
#include "Utilities.h"  // for PERCENT
class QObject;          // lines 22-22

#include <qjsondocument.h>  // for QJsonDocument
#include <qjsonobject.h>    // for QJsonObject
#include <qundostack.h>     // for QUndoCommand, QUndoStack

#include <csound/csound.hpp>  // for CSOUND
#include <csound/csPerfThread.hpp>

class QByteArray;

const auto DEFAULT_STARTING_KEY = 220;
const auto DEFAULT_STARTING_VOLUME = 50;
const auto DEFAULT_STARTING_TEMPO = 200;
const auto MINIMUM_STARTING_KEY = 60;
const auto MAXIMUM_STARTING_KEY = 440;
const auto MINIMUM_STARTING_VOLUME = 1;
const auto MAXIMUM_STARTING_VOLUME = 100;
const auto MINIMUM_STARTING_TEMPO = 100;
const auto MAXIMUM_STARTING_TEMPO = 800;

const auto SECONDS_PER_MINUTE = 60;
const auto FULL_NOTE_VOLUME = 0.2;

const auto DEFAULT_ORCHESTRA_TEXT = R""""(nchnls = 2
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
endin)"""";

const auto DEFAULT_STARTING_INSTRUMENT = "Plucked";

class Song {

 public:
  double starting_key = DEFAULT_STARTING_KEY;
  double starting_volume = DEFAULT_STARTING_VOLUME;
  double starting_tempo = DEFAULT_STARTING_TEMPO;
  QString starting_instrument;
  std::vector<std::unique_ptr<const QString>> instrument_pointers;
  QString orchestra_code;
  Csound& csound_session;
  QUndoStack& undo_stack;
  
  const QPointer<ChordsModel> chords_model_pointer =
      new ChordsModel(instrument_pointers, undo_stack);

  

  explicit Song(
    Csound& csound_session_input, QUndoStack& undo_stack_input, const QString &starting_instrument_input = DEFAULT_STARTING_INSTRUMENT,
                const QString &orchestra_code_input = DEFAULT_ORCHESTRA_TEXT);


  [[nodiscard]] auto to_json() const -> QJsonDocument;

  [[nodiscard]] auto load_from(const QByteArray &song_text) -> bool;

  
  
  
  
  void set_orchestra_text(const QString &new_orchestra_text);
  [[nodiscard]] auto verify_json(const QJsonObject &json_song) -> bool;
  [[nodiscard]] auto verify_orchestra_text_compiles(
      const QString &new_orchestra_text) -> bool;
};


