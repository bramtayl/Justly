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
  std::unique_ptr<Song> song_pointer = std::make_unique<Song>();

  gsl::not_null<QDoubleSpinBox *> starting_tempo_editor_pointer =
      new QDoubleSpinBox(this);

  gsl::not_null<QDoubleSpinBox *> starting_volume_editor_pointer =
      new QDoubleSpinBox(this);

  gsl::not_null<QDoubleSpinBox *> starting_key_editor_pointer =
      new QDoubleSpinBox(this);

  gsl::not_null<InstrumentEditor *> starting_instrument_editor_pointer =
      new InstrumentEditor(this, false);

  gsl::not_null<QAction *> insert_before_action_pointer =
      new QAction(tr("&Before"), this);

  gsl::not_null<QAction *> insert_after_action_pointer =
      new QAction(tr("&After"), this);

  gsl::not_null<QAction *> insert_into_action_pointer =
      new QAction(tr("&Into"), this);

  gsl::not_null<QAction *> remove_action_pointer =
      new QAction(tr("&Remove"), this);

  gsl::not_null<QAction *> copy_action_pointer = new QAction(tr("&Copy"), this);

  gsl::not_null<QAction *> paste_before_action_pointer =
      new QAction(tr("&Before"), this);

  gsl::not_null<QAction *> paste_after_action_pointer =
      new QAction(tr("&After"), this);

  gsl::not_null<QAction *> paste_into_action_pointer =
      new QAction(tr("&Into"), this);

  gsl::not_null<QAction *> save_action_pointer = new QAction(tr("&Save"), this);

  gsl::not_null<QAction *> play_action_pointer =
      new QAction(tr("&Play selection"), this);

  gsl::not_null<MyDelegate *> my_delegate_pointer = new MyDelegate(this);

  gsl::not_null<QTreeView *> chords_view_pointer = new QTreeView(this);

  gsl::not_null<QUndoStack *> undo_stack_pointer = new QUndoStack(this);

  gsl::not_null<ChordsModel *> chords_model_pointer = new ChordsModel(
      song_pointer.get(), undo_stack_pointer, chords_view_pointer);

  QString current_file = "";
  QString current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  std::unique_ptr<Player> player_pointer =
      std::make_unique<Player>(song_pointer.get());

  TreeLevel copy_level = root_level;

  void export_recording();
  void open();
  void save_as();

  void save_starting_key(int);
  void save_starting_volume(int);
  void save_starting_tempo(int);
  void save_starting_instrument(int);
  void save_starting_value(StartingFieldId, const QVariant &);

  void initialize_controls() const;
  void initialize_control(StartingFieldId) const;

  void fix_selection(const QItemSelection &, const QItemSelection &);

  void insert(int, int, const QModelIndex &);
  void paste(int, const QModelIndex &);

  void update_actions();
  void starting_block_signal(StartingFieldId, bool) const;

 public:
  ~Editor() override;

  // prevent moving and copying;
  Editor(const Editor &) = delete;
  auto operator=(const Editor &) -> Editor = delete;
  Editor(Editor &&) = delete;
  auto operator=(Editor &&) -> Editor = delete;

  [[nodiscard]] auto get_chords_model_pointer() const
      -> gsl::not_null<ChordsModel *>;

  explicit Editor(QWidget * = nullptr, Qt::WindowFlags = Qt::WindowFlags());

  void export_recording_to(const QString &);

  void open_file(const QString &);
  void save_as_file(const QString &);

  void paste_text(int, const QByteArray &, const QModelIndex &);

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

  void play(int, int, const QModelIndex &) const;
  void stop_playing() const;
  [[nodiscard]] auto has_real_time() const -> bool;

  [[nodiscard]] auto get_delegate_pointer() const
      -> gsl::not_null<const MyDelegate *>;

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString &;

  [[nodiscard]] auto get_control_value(StartingFieldId) const -> QVariant;

  void set_control_no_signals(StartingFieldId, const QVariant &) const;
  void set_starting_value(StartingFieldId, const QVariant &) const;
  void set_control(StartingFieldId, const QVariant &) const;
  auto get_selector_pointer() -> gsl::not_null<QItemSelectionModel *>;
  [[nodiscard]] auto get_viewport_pointer() const -> gsl::not_null<QWidget *>;
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_starting_value(StartingFieldId value_type) const
      -> QVariant;
  [[nodiscard]] auto get_number_of_children(int chord_number) const -> int;
};
