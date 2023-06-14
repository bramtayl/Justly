#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>          // for DisplayRole, ItemFlags, Orientation
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT, signals
#include <qvariant.h>            // for QVariant
#include <stddef.h>              // for size_t
#include <QJsonDocument>

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "/home/brandon/Justly/src/Song.h"
#include "TreeNode.h"  // for TreeNode
class QObject;         // lines 22-22

#include <QUndoStack>

#include <csound/csound.hpp>  // for CSOUND
#include <csound/csPerfThread.hpp>

const int DEFAULT_FREQUENCY = 220;
const int DEFAULT_STARTING_VOLUME_PERCENT = 50;
const int DEFAULT_TEMPO = 200;
const auto MIN_FREQUENCY = 60;
const auto MAX_FREQUENCY = 440;
const auto MIN_VOLUME_PERCENT = 0;
const auto MAX_VOLUME_PERCENT = 100;
const auto MIN_TEMPO = 100;
const auto MAX_TEMPO = 800;

const auto PERCENT = 100;
const auto SECONDS_PER_MINUTE = 60;
const auto FULL_NOTE_VOLUME = 0.2;

const int NOTE_CHORD_COLUMNS = 9;

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

const auto DEFAULT_DEFAULT_INSTRUMENT = "Plucked";

class Song : public QAbstractItemModel {
  Q_OBJECT

 public:
  int frequency = DEFAULT_FREQUENCY;
  int volume_percent = DEFAULT_STARTING_VOLUME_PERCENT;
  int tempo = DEFAULT_TEMPO;
  QString default_instrument = DEFAULT_DEFAULT_INSTRUMENT;
  std::vector<std::unique_ptr<const QString>> instrument_pointers;
  QString orchestra_text = DEFAULT_ORCHESTRA_TEXT;
  Csound csound_session;
  CsoundPerformanceThread performance_thread = CsoundPerformanceThread(&csound_session);
  QUndoStack undo_stack;

  double key = DEFAULT_FREQUENCY;
  double current_volume = (1.0 * DEFAULT_STARTING_VOLUME_PERCENT) / PERCENT;
  double current_tempo = DEFAULT_TEMPO;
  double current_time = 0.0;

  // pointer so the pointer, but not object, can be constant
  TreeNode root;

  explicit Song(QObject *parent = nullptr);
  ~Song() override;

  [[nodiscard]] auto node_from_index(const QModelIndex &index) -> TreeNode &;
  [[nodiscard]] auto const_node_from_index(const QModelIndex &index) const
      -> const TreeNode &;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const
      -> QVariant override;
  [[nodiscard]] auto index(int row, int column,
                           const QModelIndex &parent = QModelIndex()) const
      -> QModelIndex override;
  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const
      -> int override;
  [[nodiscard]] auto columnCount(
      const QModelIndex &parent = QModelIndex()) const -> int override;
  void setData_directly(const QModelIndex &index, const QVariant &new_value);
  auto insertRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  auto insert_children(size_t position,
                       std::vector<std::unique_ptr<TreeNode>> &insertion,
                       const QModelIndex &parent_index) -> void;
  auto removeRows_internal(size_t position, size_t rows,
                           const QModelIndex &index = QModelIndex()) -> void;
  auto removeRows(int position, int rows,
                  const QModelIndex &index = QModelIndex()) -> bool override;
  auto remove_save(size_t position, size_t rows,
                   const QModelIndex &parent_index,
                   std::vector<std::unique_ptr<TreeNode>> &deleted_rows)
      -> void;

  auto to_json() -> QJsonDocument;
  auto setData(const QModelIndex &index, const QVariant &new_value, int role)
      -> bool override;

  auto load_from(const QByteArray &song_text) -> bool;

  void redisplay();

  auto verify_instruments(
      std::vector<std::unique_ptr<const QString>> &new_instrument_pointers, bool interactive)
      -> bool;
  void play(int position, size_t rows, const QModelIndex &parent_index);
  void stop_playing();
  void update_with_chord(const TreeNode &node);
  auto get_beat_duration() const -> double;
  void schedule_note(const TreeNode &node);
  void save_orchestra_text(const QString& new_orchestra_text);
  auto verify_orchestra_text(const QString& new_orchestra_text) -> bool;
  void set_orchestra_text(const QString& new_orchestra_text);
  auto verify_json(const QJsonObject& json_song) -> bool;
  auto verify_orchestra_text_compiles(const QString& new_orchestra_text) -> bool;
};

class CellChange : public QUndoCommand {
 public:
  Song &song;
  const QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;
  CellChange(Song &song_input, const QModelIndex &index_input,
             QVariant new_value_input, QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};
