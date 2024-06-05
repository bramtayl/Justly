#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QAbstractIte...
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for WindowFlags
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <memory>  // for unique_ptr
#include <string>  // for string

#include "justly/Song.h"         // for StartingField, Song
#include "justly/StartingField.h"
#include "justly/TreeLevel.h"        // for TreeLevel

class ChordsModel;
class InstrumentEditor;
class Player;
class QAbstractItemView;
class QAction;
class QDoubleSpinBox;
class QItemSelection;
class QUndoStack;
class QWidget;

class SongEditor : public QMainWindow {
  Q_OBJECT

  Song song;

  QDoubleSpinBox* starting_tempo_editor_pointer;
  QDoubleSpinBox* starting_volume_editor_pointer;
  QDoubleSpinBox* starting_key_editor_pointer;
  InstrumentEditor* starting_instrument_editor_pointer;

  QAction* insert_before_action_pointer;
  QAction* insert_after_action_pointer;
  QAction* insert_into_action_pointer;
  QAction* remove_action_pointer;
  QAction* copy_action_pointer;
  QAction* paste_before_action_pointer;
  QAction* paste_after_action_pointer;
  QAction* paste_into_action_pointer;
  QAction* save_action_pointer;
  QAction* play_action_pointer;

  QAbstractItemView* chords_view_pointer;

  QUndoStack* undo_stack_pointer;

  ChordsModel* chords_model_pointer;

  QString current_file;
  QString current_folder;

  std::unique_ptr<Player> player_pointer;

  TreeLevel copy_level;

  void export_recording();
  void open();
  void save_as();

  void save_starting_key(int);
  void save_starting_volume(int);
  void save_starting_tempo(int);
  void save_starting_instrument(int);
  void save_starting_value(StartingField, const QVariant&);

  void initialize_controls();

  void fix_selection(const QItemSelection&, const QItemSelection&);

  void insert(int, int, const QModelIndex&);
  void paste(int, const QModelIndex&);

  void update_actions();

 public:
  ~SongEditor() override;

  // prevent moving and copying;
  SongEditor(const SongEditor&) = delete;
  auto operator=(const SongEditor&) -> SongEditor = delete;
  SongEditor(SongEditor&&) = delete;
  auto operator=(SongEditor&&) -> SongEditor = delete;

  [[nodiscard]] auto get_chords_model_pointer() const -> QAbstractItemModel*;

  explicit SongEditor(QWidget* = nullptr, Qt::WindowFlags = Qt::WindowFlags());

  void export_recording_to(const QString&);

  void open_file(const QString&);
  void save_as_file(const QString&);

  void paste_text(int, const std::string&, const QModelIndex&);

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

  void play(int, int, const QModelIndex&) const;
  void stop_playing() const;

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString&;

  void set_starting_control(StartingField, const QVariant&,
                            bool no_signals = false);
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_starting_value(StartingField value_type) const
      -> QVariant;
  [[nodiscard]] auto get_number_of_children(int chord_number) const -> int;
  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
};
