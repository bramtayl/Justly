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

#include <gsl/pointers>
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
  gsl::not_null<QMenu*> file_menu_pointer =
      std::make_unique<QMenu>(tr("&File"), this).release();
  gsl::not_null<QAction*> save_action_pointer =
      std::make_unique<QAction>(tr("&Save"), file_menu_pointer).release();
  gsl::not_null<QAction*> save_as_action_pointer =
      std::make_unique<QAction>(tr("&Save As..."), file_menu_pointer).release();
  gsl::not_null<QWidget*> central_widget_pointer =
      std::make_unique<QWidget>(this).release();
  gsl::not_null<QMenu*> edit_menu_pointer =
      std::make_unique<QMenu>(tr("&Edit"), this).release();
  gsl::not_null<QAction*> undo_action_pointer =
      std::make_unique<QAction>(tr("&Undo"), edit_menu_pointer).release();
  gsl::not_null<QAction*> copy_action_pointer =
      std::make_unique<QAction>(tr("&Copy"), edit_menu_pointer).release();
  gsl::not_null<QMenu*> paste_menu_pointer =
      std::make_unique<QMenu>(tr("&Paste"), edit_menu_pointer).release();
  gsl::not_null<QAction*> paste_before_action_pointer =
      std::make_unique<QAction>(tr("&Before"), paste_menu_pointer).release();
  gsl::not_null<QAction*> paste_after_action_pointer =
      std::make_unique<QAction>(tr("&After"), paste_menu_pointer).release();
  gsl::not_null<QAction*> paste_into_action_pointer =
      std::make_unique<QAction>(tr("&Into"), paste_menu_pointer).release();

  gsl::not_null<QMenu*> insert_menu_pointer =
      std::make_unique<QMenu>(tr("&Insert"), edit_menu_pointer).release();

  gsl::not_null<QAction*> insert_before_action_pointer =
      std::make_unique<QAction>(tr("&Before"), insert_menu_pointer).release();
  gsl::not_null<QAction*> insert_after_action_pointer =
      std::make_unique<QAction>(tr("&After"), insert_menu_pointer).release();
  gsl::not_null<QAction*> insert_into_action_pointer =
      std::make_unique<QAction>(tr("&Into"), insert_menu_pointer).release();
  gsl::not_null<QAction*> remove_action_pointer =
      std::make_unique<QAction>(tr("&RemoveChange"), edit_menu_pointer)
          .release();

  gsl::not_null<QMenu*> view_menu_pointer =
      std::make_unique<QMenu>(tr("&View"), this).release();

  gsl::not_null<QAction*> view_controls_checkbox_pointer =
      std::make_unique<QAction>(tr("&Controls"), view_menu_pointer).release();

  gsl::not_null<QMenu*> play_menu_pointer =
      std::make_unique<QMenu>(tr("&Play"), this).release();

  gsl::not_null<QAction*> play_selection_action_pointer =
      std::make_unique<QAction>(tr("&Play selection"), play_menu_pointer)
          .release();
  gsl::not_null<QAction*> stop_playing_action_pointer =
      std::make_unique<QAction>(tr("&Stop playing"), play_menu_pointer)
          .release();
  gsl::not_null<QWidget*> controls_pointer =
      std::make_unique<QWidget>(central_widget_pointer).release();

  gsl::not_null<ShowSlider*> starting_tempo_editor_pointer =
      std::make_unique<ShowSlider>(MINIMUM_STARTING_TEMPO,
                                   MAXIMUM_STARTING_TEMPO, " bpm",
                                   controls_pointer)
          .release();

  gsl::not_null<ShowSlider*> starting_volume_editor_pointer =
      std::make_unique<ShowSlider>(MINIMUM_STARTING_VOLUME,
                                   MAXIMUM_STARTING_VOLUME, "%",
                                   controls_pointer)
          .release();

  gsl::not_null<ShowSlider*> starting_key_editor_pointer =
      std::make_unique<ShowSlider>(MINIMUM_STARTING_KEY, MAXIMUM_STARTING_KEY,
                                   " Hz", controls_pointer)
          .release();

  gsl::not_null<InstrumentEditor*> starting_instrument_editor_pointer =
      std::make_unique<InstrumentEditor>(controls_pointer).release();

  gsl::not_null<ShowSliderDelegate*> tempo_percent_delegate_pointer =
      std::make_unique<ShowSliderDelegate>(MINIMUM_TEMPO_PERCENT,
                                           MAXIMUM_TEMPO_PERCENT, "%")
          .release();

  QString current_file = "";

  gsl::not_null<ShowSliderDelegate*> volume_percent_delegate_pointer =
      std::make_unique<ShowSliderDelegate>(MINIMUM_VOLUME_PERCENT,
                                           MAXIMUM_VOLUME_PERCENT, "%")
          .release();

  gsl::not_null<QClipboard*> clipboard_pointer = QGuiApplication::clipboard();
  gsl::not_null<QUndoStack*> undo_stack_pointer =
      std::make_unique<QUndoStack>(this).release();

  bool unsaved_changes = false;
  void view_controls() const;
  int copy_level = 0;

  void export_recording();
  void open();
  void save_as();
  void change_file_to(const QString& filename);

  void set_starting_key();
  void set_starting_volume();
  void set_song_starting_tempo();

  void update_selection_and_actions() const;

  void insert(int first_index, int number_of_children,
              const QModelIndex& parent_index);
  void paste(int first_index, const QModelIndex& parent_index);

  void save_starting_instrument(int new_index);

  void data_set(const QModelIndex& index, const QVariant& old_value,
                const QVariant& new_value);

 public:
  gsl::not_null<Song*> song_pointer;

  gsl::not_null<QTreeView*> chords_view_pointer =
      std::make_unique<QTreeView>(this).release();
  gsl::not_null<ChordsModel*> chords_model_pointer =
      std::make_unique<ChordsModel>(&song_pointer->root, chords_view_pointer)
          .release();

  // addMenu will take ownership, so we don't have to worry about freeing
  // setLayout will take ownership, so we don't have to worry about freeing
  gsl::not_null<IntervalDelegate*> interval_delegate_pointer =
      std::make_unique<IntervalDelegate>().release();
  gsl::not_null<SpinBoxDelegate*> beats_delegate_pointer =
      std::make_unique<SpinBoxDelegate>(MINIMUM_BEATS, MAXIMUM_BEATS).release();
  gsl::not_null<InstrumentDelegate*> instrument_delegate_pointer =
      std::make_unique<InstrumentDelegate>().release();

  explicit Editor(gsl::not_null<Song*> song_pointer, QWidget* parent = nullptr,
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
                               bool should_set_box) const;

  void play(int first_index, int number_of_children,
            const QModelIndex& parent_index) const;
  void stop_playing() const;
  [[nodiscard]] auto are_controls_visible() const -> bool;
  [[nodiscard]] auto has_real_time() const -> bool;
  void set_controls_visible(bool is_visible) const;

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString&;
  void set_current_file(const QString& new_current_file);
  [[nodiscard]] auto create_volume_slider_pointer(
      const QModelIndex& cell_index) const -> ShowSlider*;
  void set_volume_with_slider_pointer(ShowSlider* show_slider_pointer,
                                      const QModelIndex& cell_index,
                                      double new_value) const;

  [[nodiscard]] auto get_starting_tempo_slider() const -> double;
  void set_starting_tempo_slider(double starting_tempo) const;
  void set_starting_tempo_slider_no_signals(double starting_tempo) const;

  [[nodiscard]] auto get_starting_volume_slider() const -> double;
  void set_starting_volume_slider(double starting_volume) const;
  void set_starting_volume_slider_no_signals(double starting_volume) const;

  [[nodiscard]] auto get_starting_key_slider() const -> double;
  void set_starting_key_slider(double starting_key) const;
  void set_starting_key_slider_no_signals(double starting_key) const;

  [[nodiscard]] auto get_starting_instrument_box() const -> const Instrument&;
  void set_starting_instrument_box(const Instrument& instrument) const;
  void set_starting_instrument_box_no_signals(
      const Instrument& instrument) const;

 private:
  std::unique_ptr<Player> player_pointer =
      std::make_unique<Player>(song_pointer);
};