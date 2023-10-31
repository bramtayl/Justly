#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QModel...
#include <qabstractitemview.h>   // for QAbstractItemView
#include <qaction.h>             // for QAction
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for WindowFlags
#include <qspinbox.h>
#include <qstandardpaths.h>  // for QStandardPaths, QStandardP...
#include <qstring.h>         // for QString
#include <qtmetamacros.h>    // for Q_OBJECT
#include <qundostack.h>      // for QUndoStack
#include <qvariant.h>        // for QVariant

#include <memory>  // for make_unique, __unique_ptr_t
#include <string>

#include "justly/editors/InstrumentEditor.h"  // for InstrumentEditor
#include "justly/main/Player.h"               // for Player
#include "justly/main/Song.h"           // for MAXIMUM_STARTING_KEY, MAXI...
#include "justly/models/ChordsModel.h"  // for ChordsModel

class QItemSelection;
class QWidget;

class SongEditor : public QMainWindow {
  Q_OBJECT

 private:
  Song song;

  QDoubleSpinBox* starting_tempo_editor_pointer = new QDoubleSpinBox(this);

  QDoubleSpinBox* starting_volume_editor_pointer = new QDoubleSpinBox(this);

  QDoubleSpinBox* starting_key_editor_pointer = new QDoubleSpinBox(this);

  InstrumentEditor* starting_instrument_editor_pointer =
      new InstrumentEditor(this, false);

  QAction* insert_before_action_pointer = new QAction(tr("&Before"), this);

  QAction* insert_after_action_pointer = new QAction(tr("&After"), this);

  QAction* insert_into_action_pointer = new QAction(tr("&Into"), this);

  QAction* remove_action_pointer = new QAction(tr("&Remove"), this);

  QAction* copy_action_pointer = new QAction(tr("&Copy"), this);

  QAction* paste_before_action_pointer = new QAction(tr("&Before"), this);

  QAction* paste_after_action_pointer = new QAction(tr("&After"), this);

  QAction* paste_into_action_pointer = new QAction(tr("&Into"), this);

  QAction* save_action_pointer = new QAction(tr("&Save"), this);

  QAction* play_action_pointer = new QAction(tr("&Play selection"), this);

  QAbstractItemView* chords_view_pointer;

  QUndoStack* undo_stack_pointer = new QUndoStack(this);

  ChordsModel* chords_model_pointer =
      new ChordsModel(&song, undo_stack_pointer, chords_view_pointer);

  QString current_file = "";
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  std::unique_ptr<Player> player_pointer = std::make_unique<Player>(&song);

  TreeLevel copy_level = root_level;

  void export_recording();
  void open();
  void save_as();

  void save_starting_key(int);
  void save_starting_volume(int);
  void save_starting_tempo(int);
  void save_starting_instrument(int);
  void save_starting_value(StartingFieldId, const QVariant&);

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
  [[nodiscard]] auto has_real_time() const -> bool;

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString&;

  void set_starting_control(StartingFieldId, const QVariant&,
                            bool no_signals = false);
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_starting_value(StartingFieldId value_type) const
      -> QVariant;
  [[nodiscard]] auto get_number_of_children(int chord_number) const -> int;
  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
};
