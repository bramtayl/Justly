#pragma once

#include <QFileDialog>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <Qt>
#include <concepts>
#include <fluidsynth.h>
#include <nlohmann/json.hpp>

#include "chord/Chord.hpp"
#include "song/ControlId.hpp"
#include "song/ModelType.hpp"

struct ChordsModel;
struct PitchedNotesModel;
struct Instrument;
struct PercussionSet;
struct PercussionInstrument;
struct UnpitchedNotesModel;
struct Rational;
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
  QList<Chord> chords;

  // mode fields
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  // folder/file fields
  QString current_folder;
  QString current_file;

  // play state fields
  QList<double> channel_schedules;

  const Instrument *current_instrument_pointer;
  const PercussionSet *current_percussion_set_pointer;
  const PercussionInstrument *current_percussion_instrument_pointer;

  double starting_time = 0;
  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

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

  // fluidsynth fields
  fluid_settings_t *const settings_pointer;
  fluid_event_t *const event_pointer;
  fluid_sequencer_t *const sequencer_pointer;
  fluid_synth_t *synth_pointer;
  const unsigned int soundfont_id;
  const fluid_seq_id_t sequencer_id;

  fluid_audio_driver_t *audio_driver_pointer = nullptr;

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
  void set_double_directly(ControlId command_id, double value) const;
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

  // play methods
  void send_event_at(double time) const;
  void start_real_time();
  void initialize_play();
  [[nodiscard]] auto
  get_open_channel_number(int chord_number, int item_number,
                          const QString &item_description) -> int;
  void change_instrument(int channel_number, short bank_number,
                         short preset_number) const;
  void update_final_time(double new_final_time);
  void modulate(const Chord &chord);
  void modulate_before_chord(int next_chord_number);
  void play_note(int channel_number, short midi_number, const Rational &beats,
                 const Rational &velocity_ratio, int time_offset,
                 int chord_number, int item_number,
                 const QString &item_description);
  void play_pitched_notes(int chord_number, const Chord &chord,
                          int first_pitched_note_number,
                          int number_of_pitched_notes);
  void play_unpitched_notes(int chord_number, const Chord &chord,
                            int first_unpitched_note_number,
                            int number_of_unpitched_notes);
  void play_chords(int first_chord_number, int number_of_chords,
                   int wait_frames = 0);
  void stop_playing() const;
  void delete_audio_driver();

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
