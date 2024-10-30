#pragma once

#include <QFileDialog>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <Qt>
#include <concepts>
#include <nlohmann/json.hpp>

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
class QLabel;
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

  void update_actions() const;

  // mode methods
  void connect_model(const QAbstractItemModel *model_pointer) const;
  void set_model(QAbstractItemModel *model_pointer) const;
  void add_edit_children_or_back(int chord_number, bool is_notes,
                                 bool backwards);
  void is_chords_now(bool is_chords) const;
  void edit_pitched_notes(int chord_number);
  void edit_unpitched_notes(int chord_number);
  void back_to_chords_directly();
  void pitched_notes_to_chords();
  void unpitched_notes_to_chords();
  void back_to_chords();

  // starting control methods
  [[nodiscard]] auto get_double(ControlId command_id) const -> double;
  void set_double_directly(ControlId command_id, double value);
  void set_double(ControlId command_id, double new_value);
  void set_gain(double new_value);
  void set_starting_key(double new_value);
  void set_starting_velocity(double new_value);
  void set_starting_tempo(double new_value);

  // insert remove methods
  void insert_row(int row_number) const;
  void paste_insert(int row_number);
  template <std::derived_from<Row> SubRow>
  auto delete_cells_template(RowsModel<SubRow> &rows_model) const;
  void delete_cells() const;
  template <std::derived_from<Row> SubRow>
  void remove_rows_template(RowsModel<SubRow> &rows_model);

  // copy paste methods
  template <std::derived_from<Row> SubRow>
  auto copy_template(RowsModel<SubRow> &rows_model, ModelType model_type) const;
  void copy() const;
  auto parse_clipboard(ModelType model_type) -> nlohmann::json;
  template <std::derived_from<Row> SubRow>
  void paste_cells_template(RowsModel<SubRow> &rows_model,
                            ModelType model_type);
  template <std::derived_from<Row> SubRow>
  void paste_insert_template(RowsModel<SubRow> &rows_model,
                             ModelType model_type, int row_number);

  // io methods
  [[nodiscard]] auto verify_discard_changes() -> bool;
  [[nodiscard]] auto
  make_file_dialog(const QString &caption, const QString &filter,
                   QFileDialog::AcceptMode accept_mode, const QString &suffix,
                   QFileDialog::FileMode file_mode) -> QFileDialog *;
  auto get_selected_file(QFileDialog *dialog_pointer) -> QString;
  void open_file(const QString &filename);
  void save_as_file(const QString &filename);
  void export_to_file(const QString &output_file);
};
