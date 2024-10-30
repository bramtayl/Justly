#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <Qt>
#include <QtGlobal>
#include <concepts>

#include "song/ControlId.hpp"
#include "song/ModelType.hpp"
#include "song/Player.hpp"
#include "song/Song.hpp"

struct ChordsModel;
struct PitchedNotesModel;
struct UnpitchedNotesModel;
struct Row;
class QAbstractItemModel;
class QAction;
class QCloseEvent;
class QDoubleSpinBox;
template <typename T> class QList;
class QTableView;
class QUndoStack;
class QWidget;

template <std::derived_from<Row> SubRow> struct RowsModel;

struct SongEditor : public QMainWindow {
  Q_OBJECT;

public:
  // data
  Song song;
  Player player = Player(this);

  // mode fields
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  // folder/file fields
  QString current_folder;
  QString current_file;

  // const fields
  QUndoStack *const undo_stack_pointer;

  // starting controls
  QDoubleSpinBox *const gain_editor_pointer;
  QDoubleSpinBox *const starting_key_editor_pointer;
  QDoubleSpinBox *const starting_velocity_editor_pointer;
  QDoubleSpinBox *const starting_tempo_editor_pointer;

  // views and models
  QLabel *const editing_chord_text_pointer;
  QTableView *const table_view_pointer;
  ChordsModel *const chords_model_pointer;
  PitchedNotesModel *const pitched_notes_model_pointer;
  UnpitchedNotesModel *const unpitched_notes_model_pointer;

  // mode actions
  QAction *const back_to_chords_action_pointer;

  // insert remove actions
  QAction *const insert_after_action_pointer;
  QAction *const insert_into_action_pointer;
  QAction *const delete_action_pointer;
  QAction *const remove_rows_action_pointer;

  // copy paste actions
  QAction *const cut_action_pointer;
  QAction *const copy_action_pointer;
  QAction *const paste_over_action_pointer;
  QAction *const paste_into_action_pointer;
  QAction *const paste_after_action_pointer;

  // play actions
  QAction *const play_action_pointer;
  QAction *const stop_playing_action_pointer;

  // io actions
  QAction *const save_action_pointer;
  QAction *const open_action_pointer;

  // methods
  explicit SongEditor(QWidget *parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  ~SongEditor() override;

  // prevent moving and copying
  SongEditor(const SongEditor &) = delete;
  auto operator=(const SongEditor &) -> SongEditor = delete;
  SongEditor(SongEditor &&) = delete;
  auto operator=(SongEditor &&) -> SongEditor = delete;

  void closeEvent(QCloseEvent *close_event_pointer) override;
};

void is_chords_now(const SongEditor &song_editor, bool is_chords);
void set_model(SongEditor &song_editor, QAbstractItemModel *model_pointer);

template <std::derived_from<Row> SubRow>
void edit_notes(SongEditor &song_editor, RowsModel<SubRow> &rows_model,
                QList<SubRow> &new_rows, int chord_number, bool is_pitched) {
  Q_ASSERT(song_editor.current_model_type == chords_type);
  Q_ASSERT(rows_model.rows_pointer == nullptr);
  song_editor.current_chord_number = chord_number;
  rows_model.set_rows_pointer(&new_rows);
  song_editor.current_model_type =
      is_pitched ? pitched_notes_type : unpitched_notes_type;
  is_chords_now(song_editor, false);

  QString label_text;
  QTextStream stream(&label_text);
  stream << SongEditor::tr("Editing ")
         << SongEditor::tr(is_pitched ? "pitched" : "unpitched")
         << SongEditor::tr(" notes for chord ") << chord_number + 1;
  song_editor.editing_chord_text_pointer->setText(label_text);

  set_model(song_editor, &rows_model);
}

void back_to_chords_directly(SongEditor &song_editor);

// starting control methods
void set_double_directly(SongEditor &song_editor, ControlId command_id,
                         double value);

// io methods
void open_file(SongEditor &song_editor, const QString &filename);
void save_as_file(SongEditor &song_editor, const QString &filename);
void export_to_file(SongEditor &song_editor, const QString &output_file);