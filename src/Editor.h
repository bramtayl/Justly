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
#include "Song.h"  // for MAXIMUM_STARTING_KEY, MAXIMUM_STARTING_TEMPO, MAX_VOLU...
#include "SpinBoxItemDelegate.h"  // for SpinBoxItemDelegate
class QByteArray;
class TreeNode;  // lines 31-31

#include <QPointer>

const auto WINDOW_WIDTH = 800;
const auto WINDOW_HEIGHT = 600;
const auto CONTROLS_WIDTH = 500;

enum Relationship {
  selection_first,
  selection_after,
  selection_into,
};

class Editor : public QMainWindow {
  Q_OBJECT
 public:
  const QPointer<Song> song_pointer = new Song();

  const QPointer<QWidget> central_box_pointer = new QWidget();
  const QPointer<QWidget> orchestra_box_pointer = new QWidget();

  const QPointer<ShowSlider> starting_key_slider_pointer =
      new ShowSlider(MINIMUM_STARTING_KEY, MAXIMUM_STARTING_KEY, " hz");
  const QPointer<ShowSlider> starting_volume_slider_pointer =
      new ShowSlider(MINIMUM_STARTING_VOLUME, MAXIMUM_STARTING_VOLUME, "%");
  const QPointer<ShowSlider> starting_tempo_slider_pointer =
      new ShowSlider(MINIMUM_STARTING_TEMPO, MAXIMUM_STARTING_TEMPO, " bpm");

  // addMenu will take ownership, so we don't have to worry about freeing
  const QPointer<QMenu> file_menu_pointer = new QMenu(tr("&File"));
  const QPointer<QMenu> edit_menu_pointer = new QMenu(tr("&Edit"));
  const QPointer<QMenu> view_menu_pointer = new QMenu(tr("&View"));
  const QPointer<QMenu> play_menu_pointer = new QMenu(tr("&Play"));
  const QPointer<QMenu> insert_menu_pointer = new QMenu(tr("&Insert"));
  const QPointer<QMenu> paste_menu_pointer = new QMenu(tr("&Paste"));

  // setLayout will take ownership, so we don't have to worry about freeing
  const QPointer<QVBoxLayout> central_column_pointer = new QVBoxLayout();
  const QPointer<QFormLayout> controls_form_pointer = new QFormLayout();
  const QPointer<QVBoxLayout> orchestra_column_pointer = new QVBoxLayout();

  const QPointer<QAction> open_action_pointer = new QAction(tr("&Open"));
  const QPointer<QAction> save_action_pointer = new QAction(tr("&Save"));

  const QPointer<QAction> undo_action_pointer = new QAction(tr("&Undo"));
  const QPointer<QAction> redo_action_pointer = new QAction(tr("&Redo"));

  const QPointer<QAction> copy_action_pointer = new QAction(tr("&Copy"));
  const QPointer<QAction> paste_before_action_pointer =
      new QAction(tr("&Before"));
  const QPointer<QAction> paste_after_action_pointer =
      new QAction(tr("&After"));
  const QPointer<QAction> paste_into_action_pointer = new QAction(tr("&Into"));

  const QPointer<QAction> insert_before_action_pointer =
      new QAction(tr("&Before"));
  const QPointer<QAction> insert_after_action_pointer =
      new QAction(tr("&After"));
  const QPointer<QAction> insert_into_action_pointer =
      new QAction(tr("&Into"));
  const QPointer<QAction> remove_action_pointer = new QAction(tr("&Remove"));

  const QPointer<QAction> view_controls_action_pointer =
      new QAction(tr("&Controls"));
  const QPointer<QAction> view_orchestra_action_pointer =
      new QAction(tr("&Orchestra"));
  const QPointer<QAction> view_chords_action_pointer =
      new QAction(tr("&Chords"));

  const QPointer<QAction> play_selection_action_pointer =
      new QAction(tr("&Play selection"));
  const QPointer<QAction> stop_playing_action_pointer =
      new QAction(tr("&Stop playing"), this);

  const QPointer<QPushButton> save_orchestra_button_pointer = new QPushButton(tr("Save orchestra"));

  const QPointer<QWidget> controls_box_pointer = new QWidget();
  const QPointer<QTextEdit> orchestra_text_edit_pointer = new QTextEdit();

  const QPointer<QTreeView> tree_view_pointer = new QTreeView();

  void set_controls_visible();
  void set_orchestra_visible();
  void set_chords_visible();

  const QPointer<QComboBox> default_instrument_selector_pointer = new QComboBox();

  const QPointer<SpinBoxItemDelegate> numerator_delegate_pointer = new SpinBoxItemDelegate(MINIMUM_NUMERATOR, MAXIMUM_NUMERATOR);
  const QPointer<SpinBoxItemDelegate> denominator_delegate_pointer = new SpinBoxItemDelegate(MINIMUM_DENOMINATOR, MAXIMUM_DENOMINATOR);
  const QPointer<SpinBoxItemDelegate> octave_delegate_pointer = new SpinBoxItemDelegate(MINIMUM_OCTAVE, MAXIMUM_OCTAVE);
  const QPointer<SpinBoxItemDelegate> beats_delegate_pointer = new SpinBoxItemDelegate(MINIMUM_BEATS, MAXIMUM_BEATS);
  const QPointer<SliderItemDelegate> volume_delegate_pointer = new SliderItemDelegate(MINIMUM_VOLUME_PERCENT, MAXIMUM_VOLUME_PERCENT, "%");
  const QPointer<SliderItemDelegate> tempo_delegate_pointer = new SliderItemDelegate(MINIMUM_TEMPO_PERCENT, MAXIMUM_TEMPO_PERCENT, "%");
  const QPointer<ComboBoxItemDelegate> instrument_delegate_pointer;

  QModelIndexList selected;
  std::vector<std::unique_ptr<TreeNode>> copied;
  int copy_level = 0;

  explicit Editor(QWidget *parent = nullptr,
                  Qt::WindowFlags flags = Qt::WindowFlags());
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
  void set_orchestra_text(const QString &new_orchestra_text,
                          const QString &new_default_instrument,
                          bool should_set_text);
  void set_default_instrument(const QString &default_instrument,
                              bool should_set_box);
};
