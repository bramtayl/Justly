#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QModelInde...
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
#include <qtextedit.h>           // for QTextEdit
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qtreeview.h>           // for QTreeView
#include <qwidget.h>             // for QWidget
#include <stddef.h>              // for size_t

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "ComboBoxItemDelegate.h"  // for ComboBoxItemDelegate
#include "ShowSlider.h"            // for ShowSlider
#include "SliderItemDelegate.h"    // for SliderItemDelegate
#include "Song.h"                  // for MAXIMUM_STARTING_KEY, MAXIMUM_STARTING_TEMPO, MAX_VOLU...
#include "SpinBoxItemDelegate.h"   // for SpinBoxItemDelegate
class QByteArray;
class TreeNode;  // lines 31-31

const auto WINDOW_WIDTH = 800;
const auto WINDOW_HEIGHT = 600;

enum Relationship {
  selection_first,
  selection_after,
  selection_into,
};

class Editor : public QMainWindow {
  Q_OBJECT
 public:
  Song song;

  QWidget central_box;
  QVBoxLayout central_column;
  QWidget orchestra_box;
  QVBoxLayout orchestra_column;

  ShowSlider starting_key_slider = ShowSlider(MINIMUM_STARTING_KEY, MAXIMUM_STARTING_KEY, " hz");
  ShowSlider starting_volume_slider =
      ShowSlider(MINIMUM_STARTING_VOLUME, MAXIMUM_STARTING_VOLUME, "%");
  ShowSlider starting_tempo_slider = ShowSlider(MINIMUM_STARTING_TEMPO, MAXIMUM_STARTING_TEMPO, " bpm");

  QMenu file_menu = QMenu(tr("&File"));
  QMenu insert_menu = QMenu(tr("&Insert"));
  QMenu paste_menu = QMenu(tr("&Paste"));
  QMenu play_menu = QMenu(tr("&Play"));
  QMenu edit_menu = QMenu(tr("&Edit"));
  QMenu view_menu = QMenu(tr("&View"));

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

  QAction view_controls_action = QAction(tr("&Controls"));
  QAction view_orchestra_action = QAction(tr("&Orchestra"));
  QAction view_chords_action = QAction(tr("&Chords"));

  QWidget conrols_box;
  QFormLayout controls_form;
  QTextEdit orchestra_text_edit;

  QTreeView tree_view;

  void set_controls_visible();
  void set_orchestra_visible();
  void set_chords_visible();

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
  void load_from(const QByteArray &song_text);

  void set_starting_key_with_slider();
  void set_starting_volume_with_slider();
  void set_starting_tempo_with_slider();

  void copy_selected();
  void copy(int position, size_t rows, const QModelIndex &parent_index);
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
  void insert(int position, int rows, const QModelIndex &parent_index);
  void paste(int position, const QModelIndex &parent_index);

  void save();
  void save_default_instrument();
  void save_orchestra_text();
  void set_orchestra_text(const QString &new_orchestra_text, const QString &new_default_instrument,
                          bool should_set_text);
  void set_default_instrument(const QString &default_instrument,
                              bool should_set_box);
};
