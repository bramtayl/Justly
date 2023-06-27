#pragma once

#include <csound/csound.hpp>       // for Csound
#include <csound/csPerfThread.hpp> // for CsoundPerformanceThread
#include <memory>                  // for unique_ptr
#include <qaction.h>               // for QAction
#include <qboxlayout.h>            // for QVBoxLayout
#include <qcombobox.h>             // for QComboBox
#include <qformlayout.h>           // for QFormLayout
#include <qlabel.h>                // for QLabel
#include <qmainwindow.h>           // for QMainWindow
#include <qmenu.h>                 // for QMenu
#include <qnamespace.h>            // for WindowFlags
#include <qpointer.h>              // for QPointer
#include <qpushbutton.h>           // for QPushButton
#include <qstring.h>               // for QString
#include <qtextedit.h>             // for QTextEdit
#include <qtmetamacros.h>          // for Q_OBJECT
#include <qtreeview.h>             // for QTreeView
#include <qundostack.h>            // for QUndoStack
#include <qwidget.h>               // for QWidget
#include <cstddef>                // for size_t
#include <vector>                  // for vector

#include "IntervalDelegate.h"
#include "ShowSlider.h"          // for ShowSlider
#include "SliderItemDelegate.h"  // for SliderItemDelegate
#include "Song.h"                // for DEFAULT_STARTING_INSTRUMENT, DEFA...
#include "SpinBoxItemDelegate.h" // for SpinBoxItemDelegate
#include "Utilities.h"           // for MAXIMUM_BEATS, MAXIMUM_DENOMINATOR

class ComboBoxItemDelegate;
class QByteArray;
class QModelIndex;
class TreeNode;

const auto STARTING_WINDOW_WIDTH = 800;
const auto STARTING_WINDOW_HEIGHT = 600;
const auto CONTROLS_WIDTH = 500;

class Editor : public QMainWindow {
  Q_OBJECT
public:
  Song song;

  Csound csound_session;
  CsoundPerformanceThread performance_thread =
      CsoundPerformanceThread(&csound_session);
  QUndoStack undo_stack;

  double current_key = DEFAULT_STARTING_KEY;
  double current_volume = (1.0 * DEFAULT_STARTING_VOLUME) / PERCENT;
  double current_tempo = DEFAULT_STARTING_TEMPO;
  double current_time = 0.0;
  QString current_instrument = DEFAULT_STARTING_INSTRUMENT;

  const QPointer<QWidget> central_widget_pointer = new QWidget();
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
  const QPointer<QAction> insert_into_action_pointer = new QAction(tr("&Into"));
  const QPointer<QAction> remove_action_pointer = new QAction(tr("&Remove"));

  const QPointer<QAction> view_controls_checkbox_pointer =
      new QAction(tr("&Controls"));
  const QPointer<QAction> view_orchestra_checkbox_pointer =
      new QAction(tr("&Orchestra"));
  const QPointer<QAction> view_chords_checkbox_pointer =
      new QAction(tr("&Chords"));

  const QPointer<QAction> play_selection_action_pointer =
      new QAction(tr("&Play selection"));
  const QPointer<QAction> stop_playing_action_pointer =
      new QAction(tr("&Stop playing"), this);

  const QPointer<QPushButton> save_orchestra_button_pointer =
      new QPushButton(tr("Save orchestra"));

  const QPointer<QWidget> controls_widget_pointer = new QWidget();
  const QPointer<QTextEdit> orchestra_editor_pointer = new QTextEdit();

  const QPointer<QTreeView> chords_view_pointer = new QTreeView();

  void view_controls();
  void view_orchestra();
  void view_chords();

  const QPointer<QComboBox> starting_instrument_selector_pointer =
      new QComboBox();

  const QPointer<IntervalDelegate> interval_delegate_pointer = new IntervalDelegate();
  const QPointer<SpinBoxItemDelegate> beats_delegate_pointer =
      new SpinBoxItemDelegate(MINIMUM_BEATS, MAXIMUM_BEATS);
  const QPointer<SliderItemDelegate> volume_percent_delegate_pointer =
      new SliderItemDelegate(MINIMUM_VOLUME_PERCENT, MAXIMUM_VOLUME_PERCENT,
                             "%");
  const QPointer<SliderItemDelegate> tempo_percent_delegate_pointer =
      new SliderItemDelegate(MINIMUM_TEMPO_PERCENT, MAXIMUM_TEMPO_PERCENT, "%");
  const QPointer<ComboBoxItemDelegate> instrument_delegate_pointer;
  

  std::vector<std::unique_ptr<TreeNode>> copied;
  int copy_level = 0;

  explicit Editor(
      const QString &starting_instrument_input = DEFAULT_STARTING_INSTRUMENT,
      const QString &orchestra_code_input = DEFAULT_ORCHESTRA_TEXT,
      QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
  void open();
  void load_from(const QByteArray &song_text);

  void set_starting_key_with_slider();
  void set_starting_volume_with_slider();
  void set_starting_tempo_with_slider();

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
  void insert(int position, int rows, const QModelIndex &parent_index);
  void paste(int position, const QModelIndex &parent_index);

  void save();
  void save_starting_instrument();
  void save_orchestra_code();
  void set_orchestra_code(const QString &new_orchestra_text,
                          const QString &new_starting_instrument,
                          bool should_set_text);
  void set_starting_instrument(const QString &starting_instrument,
                               bool should_set_box);
  void stop_playing();

  void play(int position, size_t rows, const QModelIndex &parent_index);
  void update_with_chord(const TreeNode &node);
  void schedule_note(const TreeNode &node);
  [[nodiscard]] auto get_beat_duration() const -> double;

  // prevent copying and moving
  ~Editor() override;
  Editor(const Editor &) = delete;
  auto operator=(const Editor &) -> Editor = delete;
  Editor(Editor &&) = delete;
  auto operator=(Editor &&) -> Editor = delete;
};
