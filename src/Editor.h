#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QModelIndexList
#include <qaction.h>             // for QAction
#include <qboxlayout.h>          // for QVBoxLayout
#include <qformlayout.h>         // for QFormLayout
#include <qlabel.h>              // for QLabel
#include <qmainwindow.h>         // for QMainWindow
#include <qmenu.h>               // for QMenu
#include <qnamespace.h>          // for Horizontal, WindowFlags
#include <qslider.h>             // for QSlider
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qtreeview.h>           // for QTreeView
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "Player.h"  // for Player
#include "Selector.h"
#include "Song.h"  // for Song
class TreeNode;    // lines 22-22

const auto WINDOW_WIDTH = 800;
const auto WINDOW_HEIGHT = 600;
const auto SONG_FIELDS = 3;

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

  QSlider frequency_slider = QSlider(Qt::Horizontal);
  QSlider volume_percent_slider = QSlider(Qt::Horizontal);
  QSlider tempo_slider = QSlider(Qt::Horizontal);

  QMenu menu_tab = QMenu(tr("&Menu"));
  QMenu insert_menu = QMenu(tr("&Insert"));
  QMenu paste_menu = QMenu(tr("&Paste"));

  QAction copy_action = QAction(tr("Copy"));
  QAction paste_before_action = QAction(tr("Before"));
  QAction paste_after_action = QAction(tr("After"));
  QAction paste_into_action = QAction(tr("Into"));

  QAction insert_before_action = QAction(tr("Before"));
  QAction insert_after_action = QAction(tr("After"));
  QAction insert_into_action = QAction(tr("Into"));
  QAction remove_action = QAction(tr("&Remove"));

  QAction play_action = QAction(tr("Play Selection"));

  QAction undo_action = QAction(tr("Undo"));
  QAction redo_action = QAction(tr("Redo"));

  QWidget sliders_box;
  QFormLayout sliders_form;
  QLabel frequency_label;
  QLabel volume_percent_label;
  QLabel tempo_label;

  QTreeView view;
  Selector selector = Selector(&song, nullptr);

  QUndoStack undo_stack;
  Player player;

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

  static auto choose_file() -> QString;

  void load(const QString &file);

  auto set_frequency() -> void;
  auto set_volume_percent() -> void;
  auto set_tempo() -> void;

  auto set_frequency_label(int value) -> void;
  auto set_volume_percent_label(int value) -> void;
  auto set_tempo_label(int value) -> void;

  void copy_selected();
  void copy(const QModelIndex &first_index, size_t rows);
  static void assert_not_empty(const QModelIndexList &selected);
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
  void play(const QModelIndex &first_index, size_t rows);
  auto setData(const QModelIndex &index, const QVariant &value, int role)
      -> bool;
  auto insert(int position, int rows, const QModelIndex &parent_index) -> bool;
  void paste(int position, const QModelIndex &parent_index);
};
