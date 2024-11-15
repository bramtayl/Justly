#pragma once

#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QString>

#include "row/RowsModel.hpp"
#include "row/chord/ChordsModel.hpp"
#include "row/note/pitched_note/PitchedNotesModel.hpp"
#include "row/note/unpitched_note/UnpitchedNote.hpp" // IWYU pragma: keep
#include "song/Player.hpp"
#include "song/Song.hpp"

class QCloseEvent;
class QDoubleSpinBox;
class QLabel;
class QTableView;
class QUndoStack;

enum ControlId {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id
};

enum ModelType { chords_type, pitched_notes_type, unpitched_notes_type };

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
  QLabel &editing_text;
  QTableView &table_view;
  ChordsModel chords_model;
  PitchedNotesModel pitched_notes_model;
  RowsModel<UnpitchedNote> unpitched_notes_model;

  // mode actions
  QAction back_to_chords_action;

  // insert remove actions
  QAction insert_after_action;
  QAction insert_into_action;
  QAction delete_action;
  QAction remove_rows_action;

  // copy paste actions
  QAction cut_action;
  QAction copy_action;
  QAction paste_over_action;
  QAction paste_into_action;
  QAction paste_after_action;

  // play actions
  QAction play_action;
  QAction stop_playing_action;

  // io actions
  QAction save_action;
  QAction open_action;

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

[[nodiscard]] auto reference_get_gain(const SongEditor &song_editor) -> double;

// io methods
void reference_open_file(SongEditor &song_editor, const QString &filename);
void reference_safe_as_file(SongEditor &song_editor, const QString &filename);