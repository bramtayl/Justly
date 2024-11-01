#pragma once

#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QtGlobal>
#include <concepts>

#include "chord/ChordsModel.hpp"
#include "pitched_note/PitchedNotesModel.hpp"
#include "rows/RowsModel.hpp"
#include "song/ControlId.hpp"
#include "song/ModelType.hpp"
#include "song/Player.hpp"
#include "song/Song.hpp"
#include "unpitched_note/UnpitchedNote.hpp" // IWYU pragma: keep

struct Row;
class QAbstractItemModel;
class QAction;
class QCloseEvent;
class QDoubleSpinBox;
class QLabel;
template <typename T> class QList;
class QTableView;
class QUndoStack;

struct SongEditor : public QMainWindow {
  Q_OBJECT;

public:
  // data
  Song song;
  Player player;

  // mode fields
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  // folder/file fields
  QString current_folder;
  QString current_file;

  // const fields
  QUndoStack &undo_stack;

  // starting controls
  QDoubleSpinBox &gain_editor;
  QDoubleSpinBox &starting_key_editor;
  QDoubleSpinBox &starting_velocity_editor;
  QDoubleSpinBox &starting_tempo_editor;

  // views and models
  QLabel &editing_chord_text;
  QTableView &table_view;
  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  RowsModel<UnpitchedNote> unpitched_notes_model;

  // mode actions
  QAction &back_to_chords_action;

  // insert remove actions
  QAction &insert_after_action;
  QAction &insert_into_action;
  QAction &delete_action;
  QAction &remove_rows_action;

  // copy paste actions
  QAction &cut_action;
  QAction &copy_action;
  QAction &paste_over_action;
  QAction &paste_into_action;
  QAction &paste_after_action;

  // play actions
  QAction &play_action;
  QAction &stop_playing_action;

  // io actions
  QAction &save_action;
  QAction &open_action;

  // methods
  explicit SongEditor();
  ~SongEditor() override;

  // prevent moving and copying
  SongEditor(const SongEditor &) = delete;
  auto operator=(const SongEditor &) -> SongEditor = delete;
  SongEditor(SongEditor &&) = delete;
  auto operator=(SongEditor &&) -> SongEditor = delete;

  void closeEvent(QCloseEvent *close_event_pointer) override;
};

void set_model(SongEditor &song_editor, QAbstractItemModel &model);

template <std::derived_from<Row> SubRow>
void set_rows(RowsModel<SubRow> &rows_model,
                QList<SubRow> &new_rows) {
  Q_ASSERT(rows_model.rows_pointer == nullptr);
  rows_model.begin_reset_model();
  rows_model.rows_pointer = &new_rows;
  rows_model.end_reset_model();
}

// starting control methods
void set_double_directly(SongEditor &song_editor, ControlId command_id,
                         double value);

// io methods
void open_file(SongEditor &song_editor, const QString &filename);
void save_as_file(SongEditor &song_editor, const QString &filename);
void export_to_file(SongEditor &song_editor, const QString &output_file);