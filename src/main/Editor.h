#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QModel...
#include <qaction.h>             // for QAction
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for WindowFlags
#include <qspinbox.h>
#include <qstandardpaths.h>  // for QStandardPaths, QStandardP...
#include <qstring.h>         // for QString
#include <qtmetamacros.h>    // for Q_OBJECT
#include <qtreeview.h>       // for QTreeView
#include <qundostack.h>      // for QUndoStack
#include <qvariant.h>        // for QVariant

#include <gsl/pointers>
#include <memory>  // for make_unique, __unique_ptr_t

#include "editors/InstrumentEditor.h"  // for InstrumentEditor
#include "main/MyDelegate.h"           // for InstrumentDelegate
#include "main/Player.h"               // for Player
#include "main/Song.h"                 // for MAXIMUM_STARTING_KEY, MAXI...
#include "models/ChordsModel.h"        // for ChordsModel
#include "notechord/NoteChord.h"       // for TreeLevel, root_level

class QByteArray;
class QItemSelectionModel;
class QItemSelection;
class QWidget;

class Editor : public QMainWindow {
  Q_OBJECT

 private:
  gsl::not_null<QDoubleSpinBox*> starting_tempo_editor_pointer =
      std::make_unique<QDoubleSpinBox>(this).release();

  gsl::not_null<QDoubleSpinBox*> starting_volume_editor_pointer =
      std::make_unique<QDoubleSpinBox>(this).release();

  gsl::not_null<QDoubleSpinBox*> starting_key_editor_pointer =
      std::make_unique<QDoubleSpinBox>(this).release();

  gsl::not_null<InstrumentEditor*> starting_instrument_editor_pointer =
      std::make_unique<InstrumentEditor>(this, false).release();

  gsl::not_null<QAction*> insert_before_action_pointer =
      std::make_unique<QAction>(tr("&Before"), this).release();

  gsl::not_null<QAction*> insert_after_action_pointer =
      std::make_unique<QAction>(tr("&After"), this).release();

  gsl::not_null<QAction*> insert_into_action_pointer =
      std::make_unique<QAction>(tr("&Into"), this).release();

  gsl::not_null<QAction*> remove_action_pointer =
      std::make_unique<QAction>(tr("&Remove"), this).release();

  gsl::not_null<QAction*> copy_action_pointer =
      std::make_unique<QAction>(tr("&Copy"), this).release();

  gsl::not_null<QAction*> paste_before_action_pointer =
      std::make_unique<QAction>(tr("&Before"), this).release();

  gsl::not_null<QAction*> paste_after_action_pointer =
      std::make_unique<QAction>(tr("&After"), this).release();

  gsl::not_null<QAction*> paste_into_action_pointer =
      std::make_unique<QAction>(tr("&Into"), this).release();

  gsl::not_null<QAction*> save_action_pointer =
      std::make_unique<QAction>(tr("&Save"), this).release();

  gsl::not_null<QAction*> play_action_pointer =
      std::make_unique<QAction>(tr("&Play selection"), this).release();

  gsl::not_null<MyDelegate*> my_delegate_pointer =
      std::make_unique<MyDelegate>(this).release();

  std::unique_ptr<Song> song_pointer = std::make_unique<Song>();

  gsl::not_null<QTreeView*> chords_view_pointer =
      std::make_unique<QTreeView>(this).release();

  gsl::not_null<QUndoStack*> undo_stack_pointer =
      std::make_unique<QUndoStack>(this).release();

  gsl::not_null<ChordsModel*> chords_model_pointer =
      std::make_unique<ChordsModel>(&song_pointer->root, undo_stack_pointer,
                                    chords_view_pointer)
          .release();

  QString current_file = "";
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  std::unique_ptr<Player> player_pointer =
      std::make_unique<Player>(song_pointer.get());

  TreeLevel copy_level = root_level;

  void export_recording();
  void open();
  void save_as();

  void save_starting_key(int new_value);
  void save_starting_volume(int new_value);
  void save_starting_tempo(int new_value);
  void save_starting_instrument(int new_index);
  void save_starting_value(StartingFieldId value_type,
                           const QVariant& new_value);

  void initialize_controls() const;
  void initialize_control(StartingFieldId value_type) const;

  void fix_selection(const QItemSelection& selected,
                     const QItemSelection& deselected);

  void insert(int first_child_number, int number_of_children,
              const QModelIndex& parent_index);
  void paste(int first_child_number, const QModelIndex& parent_index);

  void update_actions();
  void starting_block_signal(StartingFieldId value_type,
                             bool should_block) const;

 public:
  [[nodiscard]] auto get_song() const -> const Song&;
  [[nodiscard]] auto get_chords_model() const -> ChordsModel&;

  explicit Editor(QWidget* parent = nullptr,
                  Qt::WindowFlags flags = Qt::WindowFlags());

  void export_recording_to(const QString& filename);

  void open_file(const QString& filename);
  void save_as_file(const QString& filename);

  void paste_text(int first_child_number, const QByteArray& paste_text,
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

  void play(int first_child_number, int number_of_children,
            const QModelIndex& parent_index) const;
  void stop_playing() const;
  [[nodiscard]] auto has_real_time() const -> bool;

  [[nodiscard]] auto get_delegate() const -> const MyDelegate&;

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString&;

  [[nodiscard]] auto get_control_value(StartingFieldId value_type) const
      -> QVariant;

  void set_control_no_signals(StartingFieldId value_type,
                              const QVariant& new_value) const;
  void set_starting_value(StartingFieldId value_type,
                          const QVariant& new_value) const;
  void set_control(StartingFieldId value_type, const QVariant& new_value) const;
  auto get_selection_model() -> QItemSelectionModel&;
  [[nodiscard]] auto get_viewport_pointer() const -> QWidget*;
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;
};
