#pragma once

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex (...
#include <qaction.h>             // for QAction
#include <qboxlayout.h>          // for QVBoxLayout
#include <qcombobox.h>           // for QComboBox
#include <qformlayout.h>         // for QFormLayout
#include <qlabel.h>              // for QLabel
#include <qmainwindow.h>         // for QMainWindow
#include <qmenu.h>               // for QMenu
#include <qnamespace.h>          // for WindowFlags
#include <qpointer.h>            // for QPointer
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qtreeview.h>           // for QTreeView
#include <qundostack.h>          // for QUndoStack
#include <qwidget.h>             // for QWidget

#include "ChordsModel.h"
#include "ComboBoxDelegate.h"    // for ComboBoxDelegate
#include "InstrumentsModel.h"    // for InstrumentsModel
#include "IntervalDelegate.h"    // for IntervalDelegate
#include "NoteChord.h"           // for MAXIMUM_BEATS, MAXIMUM_TEMPO_PERCENT
#include "Player.h"
#include "ShowSlider.h"          // for ShowSlider
#include "ShowSliderDelegate.h"  // for ShowSliderDelegate
#include "Song.h"                // for DEFAULT_STARTING_INSTRUMENT, Song
#include "SpinBoxDelegate.h"     // for SpinBoxDelegate

class QByteArray;
class QClipboard;

const auto STARTING_WINDOW_WIDTH = 800;
const auto STARTING_WINDOW_HEIGHT = 600;
const auto CONTROLS_WIDTH = 500;

class Editor : public QMainWindow {
  Q_OBJECT
 public:
  
  Song& song;

  std::unique_ptr<Player> player_pointer = std::make_unique<Player>(song);
  QClipboard* const clipboard_pointer;
  QUndoStack undo_stack;
  const QPointer<ChordsModel> chords_model_pointer =
      new ChordsModel(song.root, *this);
  bool unsaved_changes = false;

  QString current_file = "";

  QPointer<QAbstractItemModel> instruments_model_pointer =
      new InstrumentsModel(song.instruments, false);

  const QPointer<QWidget> central_widget_pointer = new QWidget();

  const QPointer<ShowSlider> starting_key_show_slider_pointer =
      new ShowSlider(MINIMUM_STARTING_KEY, MAXIMUM_STARTING_KEY, " Hz");
  const QPointer<ShowSlider> starting_volume_show_slider_pointer =
      new ShowSlider(MINIMUM_STARTING_VOLUME, MAXIMUM_STARTING_VOLUME, "%");
  const QPointer<ShowSlider> starting_tempo_show_slider_pointer =
      new ShowSlider(MINIMUM_STARTING_TEMPO, MAXIMUM_STARTING_TEMPO, " bpm");

  // addMenu will take ownership, so we don't have to worry about freeing
  const QPointer<QMenu> file_menu_pointer = new QMenu(tr("&File"));
  const QPointer<QMenu> edit_menu_pointer = new QMenu(tr("&Edit"));
  const QPointer<QMenu> view_menu_pointer = new QMenu(tr("&View"));
  const QPointer<QMenu> play_menu_pointer = new QMenu(tr("&Play"));
  const QPointer<QMenu> insert_menu_pointer = new QMenu(tr("&Insert"));
  const QPointer<QMenu> paste_menu_pointer = new QMenu(tr("&Paste"));

  const QPointer<QLabel> starting_key_label_pointer =
      new QLabel(tr("Starting key"));
  const QPointer<QLabel> starting_volume_label_pointer =
      new QLabel(tr("Starting volume"));
  const QPointer<QLabel> starting_tempo_label_pointer =
      new QLabel(tr("Starting tempo"));
  const QPointer<QLabel> starting_instrument_label_pointer =
      new QLabel(tr("Starting instrument"));

  // setLayout will take ownership, so we don't have to worry about freeing
  const QPointer<QVBoxLayout> central_layout_pointer = new QVBoxLayout();
  const QPointer<QFormLayout> controls_form_pointer = new QFormLayout();

  const QPointer<QAction> open_action_pointer = new QAction(tr("&Open"));
  const QPointer<QAction> export_as_action_pointer = new QAction(tr("&Export recording"));
  const QPointer<QAction> save_action_pointer = new QAction(tr("&Save"));
  const QPointer<QAction> save_as_action_pointer = new QAction(tr("&Save As..."));

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
  const QPointer<QAction> insert_into_action_pointer = new QAction(tr("&Into"));
  const QPointer<QAction> remove_action_pointer = new QAction(tr("&Remove"));

  const QPointer<QAction> view_controls_checkbox_pointer =
      new QAction(tr("&Controls"));

  const QPointer<QAction> play_selection_action_pointer =
      new QAction(tr("&Play selection"));
  const QPointer<QAction> stop_playing_action_pointer =
      new QAction(tr("&Stop playing"), this);

  const QPointer<QWidget> controls_pointer = new QWidget();
  const QPointer<QTreeView> chords_view_pointer = new QTreeView();

  void view_controls();

  const QPointer<QComboBox> starting_instrument_selector_pointer =
      new QComboBox();

  const QPointer<IntervalDelegate> interval_delegate_pointer =
      new IntervalDelegate();
  const QPointer<SpinBoxDelegate> beats_delegate_pointer =
      new SpinBoxDelegate(MINIMUM_BEATS, MAXIMUM_BEATS);
  const QPointer<ShowSliderDelegate> volume_percent_delegate_pointer =
      new ShowSliderDelegate(MINIMUM_VOLUME_PERCENT, MAXIMUM_VOLUME_PERCENT,
                             "%");
  const QPointer<ShowSliderDelegate> tempo_percent_delegate_pointer =
      new ShowSliderDelegate(MINIMUM_TEMPO_PERCENT, MAXIMUM_TEMPO_PERCENT, "%");
  const QPointer<ComboBoxDelegate> instrument_delegate_pointer =
      new ComboBoxDelegate(new InstrumentsModel(song.instruments, true));

  int copy_level = 0;

  explicit Editor(
      Song& song,
      QWidget *parent = nullptr,
      Qt::WindowFlags flags = Qt::WindowFlags());

  void export_recording();
  void export_recording_file(const QString& filename);
  void open();
  void open_file(const QString& filename);
  void register_changed();
  void save_as();
  void save_as_file(const QString& filename);
  void change_file_to(const QString& filename);
  
  void load_text(const QByteArray &song_text);
  void paste_text(int first_index, const QByteArray &paste_text, const QModelIndex &parent_index);

  void set_starting_key();
  void set_starting_volume();
  void set_starting_tempo();

  void copy_selected();
  void insert_before();
  void insert_after();
  void insert_into();

  void paste_before();
  void paste_after();
  void paste_into();
  
  void update_selection_and_actions();
  void remove_selected();
  void play_selected();
  void insert(int first_index, int number_of_children, const QModelIndex &parent_index);
  void paste(int first_index, const QModelIndex &parent_index);

  void save();
  void save_starting_instrument(int new_index);
  void set_starting_instrument(const QString &new_starting_instrument,
                               bool should_set_box);

  void play(int first_index, int number_of_children, const QModelIndex &parent_index);
  void stop_playing();
  
};
