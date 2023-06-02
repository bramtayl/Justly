#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QModelIndexList
#include <qaction.h>             // for QAction
#include <qboxlayout.h>          // for QVBoxLayout
#include <qcombobox.h>           // for QComboBox
#include <qformlayout.h>         // for QFormLayout
#include <qlabel.h>              // for QLabel
#include <qmainwindow.h>         // for QMainWindow
#include <qmenu.h>               // for QMenu
#include <qnamespace.h>          // for WindowFlags
#include <qpushbutton.h>         // for QPushButton
#include <qstring.h>             // for QString
#include <qtemporaryfile.h>      // for QTemporaryFile
#include <qtextedit.h>           // for QTextEdit
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qtreeview.h>           // for QTreeView
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget
#include <stddef.h>              // for size_t

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "ComboBoxItemDelegate.h"  // for ComboBoxItemDelegate
#include "CsoundData.h"            // for CsoundData
#include "Selector.h"              // for Selector
#include "ShowSlider.h"            // for ShowSlider
#include "SliderItemDelegate.h"    // for SliderItemDelegate
#include "Song.h"                  // for Song, DEFAULT_FREQUENCY, DEFAULT_S...
#include "SpinBoxItemDelegate.h"   // for SpinBoxItemDelegate

class QTextStream;  // lines 30-30
class TreeNode;     // lines 31-31

const auto WINDOW_WIDTH = 800;
const auto WINDOW_HEIGHT = 600;
const auto SONG_FIELDS = 3;

const auto PERCENT = 100;
const auto FRAMES_PER_BUFFER = 256;
const auto SECONDS_PER_MINUTE = 60;
const auto MILLISECONDS_PER_SECOND = 1000;
const auto FULL_NOTE_VOLUME = 0.2;

enum Relationship {
  selection_first,
  selection_after,
  selection_into,
};

class Editor : public QMainWindow {
  Q_OBJECT
 public:
  Song song;

  double key = DEFAULT_FREQUENCY;
  double current_volume = (1.0 * DEFAULT_STARTING_VOLUME_PERCENT) / PERCENT;
  double current_tempo = DEFAULT_TEMPO;
  double current_time = 0.0;

  CsoundData csound_data;

  QTemporaryFile score_file;
  QTemporaryFile orchestra_file;

  QWidget central_box;
  QVBoxLayout central_column;

  ShowSlider frequency_slider = ShowSlider(MIN_FREQUENCY, MAX_FREQUENCY, " hz");
  ShowSlider volume_percent_slider =
      ShowSlider(MIN_VOLUME_PERCENT, MAX_VOLUME_PERCENT, "%");
  ShowSlider tempo_slider = ShowSlider(MIN_TEMPO, MAX_TEMPO, " bpm");

  QMenu file_menu = QMenu(tr("&File"));
  QMenu insert_menu = QMenu(tr("&Insert"));
  QMenu paste_menu = QMenu(tr("&Paste"));
  QMenu play_menu = QMenu(tr("&Play"));
  QMenu edit_menu = QMenu(tr("&Edit"));

  QAction open_action = QAction(tr("&Open"));
  QAction save_action = QAction(tr("&Save"));

  QAction copy_action = QAction(tr("&Copy"));
  QAction paste_before_action = QAction(tr("&Before"));
  QAction paste_after_action = QAction(tr("&After"));
  QAction paste_into_action = QAction(tr("&Into"));

  QAction insert_before_action = QAction(tr("&Before"));
  QAction insert_after_action = QAction(tr("&After"));
  QAction insert_into_action = QAction(tr("&Into"));
  QAction remove_action = QAction(tr("&Remove"));

  QAction play_action = QAction(tr("&Play selection"));
  QAction stop_action = QAction(tr("&Stop playing"));

  QAction undo_action = QAction(tr("&Undo"));
  QAction redo_action = QAction(tr("&Redo"));
  QPushButton save_orchestra_button = QPushButton(tr("Save orchestra"));

  QWidget sliders_box;
  QFormLayout sliders_form;
  QLabel frequency_label = QLabel("Frequency:");
  QLabel default_instrument_label = QLabel("Default instrument:");
  QLabel volume_percent_label = QLabel("Volume Percent:");
  QLabel tempo_label = QLabel("Tempo:");
  QLabel orchestra_text_label = QLabel("CSound orchestra:");
  QTextEdit orchestra_text_edit;

  QTreeView view;
  Selector selector = Selector(&song, nullptr);

  QUndoStack undo_stack;

  QComboBox default_instrument_selector;

  SpinBoxItemDelegate numerator_delegate = SpinBoxItemDelegate(-99, 99);
  SpinBoxItemDelegate denominator_delegate = SpinBoxItemDelegate(-99, 99);
  SpinBoxItemDelegate octave_delegate = SpinBoxItemDelegate(-99, 99);
  SpinBoxItemDelegate beats_delegate = SpinBoxItemDelegate(0, 99);
  SliderItemDelegate volume_delegate = SliderItemDelegate(1, 200, "%");
  SliderItemDelegate tempo_delegate = SliderItemDelegate(1, 200, "%");
  ComboBoxItemDelegate instrument_delegate;

  QModelIndexList selected;
  std::vector<std::unique_ptr<TreeNode>> copied;
  int copy_level = 0;

  explicit Editor(QWidget *parent = nullptr,
                  Qt::WindowFlags flags = Qt::WindowFlags());
  ~Editor() override;
  Editor(const Editor &other) = delete;
  auto operator=(const Editor &other) -> Editor & = delete;
  Editor(Editor &&other) = delete;
  auto operator=(Editor &&other) -> Editor & = delete;

  void open();
  void load_from(const QString &file);

  auto set_frequency_with_slider() -> void;
  auto set_volume_percent_with_slider() -> void;
  auto set_tempo_with_slider() -> void;

  void copy_selected();
  void copy(int position, size_t rows, const QModelIndex &parent_index);
  [[nodiscard]] auto first_selected_index() -> QModelIndex;
  [[nodiscard]] auto last_selected_index() -> QModelIndex;
  [[nodiscard]] auto selection_parent_or_root_index() -> QModelIndex;

  void insert_before();
  void insert_after();
  void insert_into();

  void paste_before();
  void paste_after();
  void paste_into();

  void reenable_actions();
  void remove(int position, size_t rows, const QModelIndex &parent_index);
  void remove_selected();
  void play_selected();
  void stop_playing();
  void play(int position, size_t rows, const QModelIndex &parent_index);
  auto setData(const QModelIndex &index, const QVariant &value) -> bool;
  auto insert(int position, int rows, const QModelIndex &parent_index) -> bool;
  void paste(int position, const QModelIndex &parent_index);

  void update_with_chord(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> double;
  void schedule_note(QTextStream &csound_io, const TreeNode &node) const;

  void save();
  void save_to(const QString &file) const;
  void save_default_instrument();
  void save_orchestra_text();
  void fill_default_instrument_options();
  void set_orchestra_text(const QString &new_orchestra_text,
                          bool should_set_text);
  void set_default_instrument(const QString &default_instrument,
                              bool should_set_box);
};
