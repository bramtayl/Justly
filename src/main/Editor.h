#pragma once

#include <qaction.h>          // for QAction
#include <qguiapplication.h>  // for QGuiApplication
#include <qmainwindow.h>      // for QMainWindow
#include <qmenu.h>            // for QMenu
#include <qnamespace.h>       // for WindowFlags
#include <qstring.h>          // for QString
#include <qtmetamacros.h>     // for Q_OBJECT
#include <qtreeview.h>        // for QTreeView
#include <qundostack.h>       // for QUndoStack
#include <qvariant.h>         // for QVariant
#include <qwidget.h>          // for QWidget

#include <memory>  // for make_unique, __unique_ptr_t

#include "delegates/InstrumentDelegate.h"  // for InstrumentDelegate
#include "delegates/IntervalDelegate.h"    // for IntervalDelegate
#include "delegates/ShowSliderDelegate.h"  // for ShowSliderDelegate
#include "delegates/SpinBoxDelegate.h"     // for SpinBoxDelegate
#include "editors/InstrumentEditor.h"      // for InstrumentEditor
#include "editors/ShowSlider.h"            // for ShowSlider
#include "main/Player.h"                   // for Player
#include "main/Song.h"                     // for MAXIMUM_STARTING_KEY, MAXI...
#include "models/ChordsModel.h"            // for ChordsModel
#include "notechord/NoteChord.h"           // for MAXIMUM_BEATS, MAXIMUM_TEM...

class Instrument;
class QByteArray;
class QClipboard;
class QModelIndex;

const auto STARTING_WINDOW_WIDTH = 800;
const auto STARTING_WINDOW_HEIGHT = 600;
const auto CONTROLS_WIDTH = 500;

class Editor : public QMainWindow {
  Q_OBJECT
 private:
  QMenu* file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();
  QAction* save_action_pointer =
      std::make_unique<QAction>(tr("&Save"), file_menu_pointer).release();
  QAction* save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  QWidget* central_widget_pointer = std::make_unique<QWidget>(this).release();
  QMenu* edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();
  QAction* undo_action_pointer =
      std::make_unique<QAction>(tr("&Undo"), edit_menu_pointer).release();
  QAction* copy_action_pointer =
      std::make_unique<QAction>(tr("&Copy"), edit_menu_pointer).release();
  QMenu* paste_menu_pointer =
      std::make_unique<QMenu>(tr("&Paste"), edit_menu_pointer).release();
  QAction* paste_before_action_pointer =
      std::make_unique<QAction>(tr("&Before"), paste_menu_pointer).release();
  QAction* paste_after_action_pointer =
      std::make_unique<QAction>(tr("&After"), paste_menu_pointer).release();
  QAction* paste_into_action_pointer =
      std::make_unique<QAction>(tr("&Into"), paste_menu_pointer).release();

  QMenu* insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  QAction* insert_before_action_pointer =
      std::make_unique<QAction>(tr("&Before"), insert_menu_pointer).release();
  QAction* insert_after_action_pointer =
      std::make_unique<QAction>(tr("&After"), insert_menu_pointer).release();
  QAction* insert_into_action_pointer =
      std::make_unique<QAction>(tr("&Into"), insert_menu_pointer).release();
  QAction* remove_action_pointer =
      std::make_unique<QAction>(tr("&RemoveChange"), edit_menu_pointer)
          .release();

  QMenu* view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  QMenu* play_menu_pointer =
      std::make_unique<QMenu>(tr("&Play"), this).release();

  QAction* play_selection_action_pointer =
      std::make_unique<QAction>(tr("&Play selection"), play_menu_pointer)
          .release();
  QAction* stop_playing_action_pointer =
      std::make_unique<QAction>(tr("&Stop playing"), play_menu_pointer)
          .release();

  ShowSliderDelegate* tempo_percent_delegate_pointer =
      std::make_unique<ShowSliderDelegate>(MINIMUM_TEMPO_PERCENT,
                                           MAXIMUM_TEMPO_PERCENT, "%")
          .release();

  QClipboard* clipboard_pointer = QGuiApplication::clipboard();
  bool unsaved_changes = false;
  void view_controls() const;
  int copy_level = 0;

  void export_recording();
  void open();
  void save_as();
  void change_file_to(const QString& filename);

  void set_starting_key();
  void set_starting_volume();
  void set_starting_tempo();

  void update_selection_and_actions() const;

  void insert(int first_index, int number_of_children,
              const QModelIndex& parent_index);
  void paste(int first_index, const QModelIndex& parent_index);

  void save_starting_instrument(int new_index);

  void data_set(const QModelIndex& index, const QVariant& old_value,
                const QVariant& new_value);
 public:
  Song* song_pointer;
  std::unique_ptr<Player> player_pointer =
      std::make_unique<Player>(song_pointer);
  QUndoStack undo_stack;
  QTreeView* chords_view_pointer = std::make_unique<QTreeView>(this).release();
  ChordsModel* chords_model_pointer =
      std::make_unique<ChordsModel>(&song_pointer->root, chords_view_pointer)
          .release();
  QString current_file = "";
  QWidget* controls_pointer =
      std::make_unique<QWidget>(central_widget_pointer).release();

  ShowSlider* starting_key_editor_pointer =
      std::make_unique<ShowSlider>(MINIMUM_STARTING_KEY, MAXIMUM_STARTING_KEY,
                                   " Hz", controls_pointer)
          .release();
  ShowSlider* starting_volume_editor_pointer =
      std::make_unique<ShowSlider>(MINIMUM_STARTING_VOLUME,
                                   MAXIMUM_STARTING_VOLUME, "%",
                                   controls_pointer)
          .release();
  ShowSlider* starting_tempo_editor_pointer =
      std::make_unique<ShowSlider>(MINIMUM_STARTING_TEMPO,
                                   MAXIMUM_STARTING_TEMPO, " bpm",
                                   controls_pointer)
          .release();
  // addMenu will take ownership, so we don't have to worry about freeing
  // setLayout will take ownership, so we don't have to worry about freeing

  QAction* view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

  InstrumentEditor* starting_instrument_editor_pointer =
      std::make_unique<InstrumentEditor>(controls_pointer).release();

  IntervalDelegate* interval_delegate_pointer =
      std::make_unique<IntervalDelegate>().release();
  SpinBoxDelegate* beats_delegate_pointer =
      std::make_unique<SpinBoxDelegate>(MINIMUM_BEATS, MAXIMUM_BEATS).release();
  ShowSliderDelegate* volume_percent_delegate_pointer =
      std::make_unique<ShowSliderDelegate>(MINIMUM_VOLUME_PERCENT,
                                           MAXIMUM_VOLUME_PERCENT, "%")
          .release();
  InstrumentDelegate* instrument_delegate_pointer =
      std::make_unique<InstrumentDelegate>().release();

  explicit Editor(Song* song_pointer, QWidget* parent = nullptr,
                  Qt::WindowFlags flags = Qt::WindowFlags());

  void export_recording_file(const QString& filename);

  void open_file(const QString& filename);
  void register_changed();

  void save_as_file(const QString& filename);

  void paste_text(int first_index, const QByteArray& paste_text,
                  const QModelIndex& parent_index);

  void copy_selected();
  void insert_before();
  void insert_after();
  void insert_into();

  void paste_before();
  void paste_after();
  void paste_into();

  void remove_selected();
  void play_selected() const;

  void save();
  void set_starting_instrument(const Instrument& new_starting_instrument,
                               bool should_set_box);

  void play(int first_index, int number_of_children,
            const QModelIndex& parent_index) const;
  void stop_playing() const;
};
