#pragma once

#include <qmainwindow.h>      // for QMainWindow
#include <qnamespace.h>       // for WindowFlags
#include <qspinbox.h>
#include <qstring.h>       // for QString
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qtreeview.h>     // for QTreeView
#include <qundostack.h>    // for QUndoStack
#include <qvariant.h>      // for QVariant

#include <gsl/pointers>
#include <memory>  // for make_unique, __unique_ptr_t

#include "editors/InstrumentEditor.h"  // for InstrumentEditor
#include "main/MyDelegate.h"           // for InstrumentDelegate
#include "main/Player.h"               // for Player
#include "main/Song.h"                 // for MAXIMUM_STARTING_KEY, MAXI...
#include "models/ChordsModel.h"        // for ChordsModel
#include "notechord/NoteChord.h"       // for TreeLevel, root_level
#include "utilities/utilities.h"

class QByteArray;
class QItemSelectionModel;
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

  gsl::not_null<MyDelegate*> my_delegate_pointer =
      std::make_unique<MyDelegate>(this).release();

  std::unique_ptr<Song> song_pointer = std::make_unique<Song>();

  gsl::not_null<QTreeView*> chords_view_pointer =
      std::make_unique<QTreeView>(this).release();

  gsl::not_null<ChordsModel*> chords_model_pointer =
      std::make_unique<ChordsModel>(&song_pointer->root, chords_view_pointer)
          .release();

  QString current_file = "";

  gsl::not_null<QUndoStack*> undo_stack_pointer =
      std::make_unique<QUndoStack>(this).release();

  std::unique_ptr<Player> player_pointer =
      std::make_unique<Player>(song_pointer.get());

  TreeLevel copy_level = root_level;
  TreeLevel selected_level = root_level;
  bool any_selected = false;
  bool empty_chord_is_selected = false;
  bool can_paste = false;
  bool can_insert_into = true;
  bool can_paste_into = false;
  bool can_save = false;
  bool can_save_as = false;

  void export_recording();
  void open();
  void save_as();

  void save_starting_key(int new_value);
  void save_starting_volume(int new_value);
  void save_starting_tempo(int new_value);

  void update_selection_and_actions();

  void insert(int first_index, int number_of_children,
              const QModelIndex& parent_index);
  void paste(int first_index, const QModelIndex& parent_index);

  void save_starting_instrument(int new_index);

  void change_cell(const QModelIndex& index, const QVariant& old_value,
                   const QVariant& new_value);
  void initialize_controls() const;
  void save_new_starting_value(StartingFieldId value_type,
                               const QVariant& new_value);

  void initialize_starting_control_value(StartingFieldId value_type) const;
  void update_clean(bool clean);
  void update_pastes();
  void starting_block_signal(StartingFieldId value_type,
                             bool should_block) const;
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

 signals:
  void anySelectedChanged(bool new_any_selected) const;
  void canPasteChanged(bool new_can_paste) const;
  void canInsertIntoChanged(bool new_can_insert_into) const;
  void canPasteIntoChanged(bool new_can_paste_into) const;
  void canSaveChanged(bool new_can_save) const;
  void canSaveAsChanged(bool new_can_save) const;

 public:
  [[nodiscard]] auto get_song() const -> const Song&;
  [[nodiscard]] auto get_chords_model() const -> ChordsModel&;

  explicit Editor(QWidget* parent = nullptr,
                  Qt::WindowFlags flags = Qt::WindowFlags());

  void export_recording_file(const QString& filename);

  void open_file(const QString& filename);
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

  void play(int first_index, int number_of_children,
            const QModelIndex& parent_index) const;
  void stop_playing() const;
  [[nodiscard]] auto has_real_time() const -> bool;

  [[nodiscard]] auto get_delegate() const -> const MyDelegate&;

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString&;
  void set_current_file(const QString& new_file);

  [[nodiscard]] auto get_starting_control_value(
      StartingFieldId value_type) const -> QVariant;

  void set_starting_control_value_no_signals(StartingFieldId value_type,
                                             const QVariant& new_value) const;
  void set_starting_control_value(StartingFieldId value_type,
                                  const QVariant& new_value) const;
  auto get_selection_model() -> QItemSelectionModel&;
  void load_text(const QString& song_text);
  [[nodiscard]] auto get_chords_viewport_pointer() const -> QWidget*;
};
