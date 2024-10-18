#pragma once

#include <QFileDialog>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <Qt>
#include <QtGlobal>
#include <fluidsynth.h>
#include <nlohmann/json.hpp>

struct Chord;
struct ChordsModel;
struct NotesModel;
struct PercussionsModel;
struct Rational;
class QAbstractItemModel;
class QAction;
class QCloseEvent;
class QDoubleSpinBox;
class QTableView;
class QUndoStack;
class QWidget;
namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

enum ModelType { chords_type, notes_type, percussion_type };

struct SongEditor : public QMainWindow {
  Q_OBJECT;

public:
  // mutable fields

  // mode fields
  ModelType current_model_type = chords_type;
  int current_chord_number = -1;

  // folder/file fields
  QString current_folder;
  QString current_file;

  // play state fields
  QList<double> channel_schedules;

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
  QTableView *const table_view_pointer;
  ChordsModel *const chords_model_pointer;
  NotesModel *const notes_model_pointer;
  PercussionsModel *const percussions_model_pointer;

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

  // const fields
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
  void edit_notes(qsizetype chord_number);
  void is_chords_now(bool is_chords) const;
  void edit_percussions(qsizetype chord_number);
  void notes_to_chords();
  void percussions_to_chords();
  void back_to_chords();

  // direct starting control methods
  void set_gain_directly(double new_gain) const;
  void set_starting_key_directly(double new_value) const;
  void set_starting_velocity_directly(double new_value) const;
  void set_starting_tempo_directly(double new_value) const;

  // indirect starting control methods
  void set_gain(double new_value);
  void set_starting_key(double new_value);
  void set_starting_velocity(double new_value);
  void set_starting_tempo(double new_value);

  // insert remove methods
  void insert_row(qsizetype row_number);
  void paste_insert(qsizetype row_number);
  void delete_cells();

  // copy paste methods
  void copy() const;
  auto parse_clipboard(const QString &mime_type,
                       const nlohmann::json_schema::json_validator &validator)
      -> nlohmann::json;

  // play methods
  void send_event_at(double time) const;
  void start_real_time();
  void initialize_play();
  [[nodiscard]] auto
  get_open_channel_number(qsizetype chord_number, qsizetype item_number,
                          const QString &item_description) -> int;
  void change_instrument(int channel_number, short bank_number,
                         short preset_number) const;
  void update_final_time(double new_final_time);
  void modulate(const Chord &chord);
  void play_note_or_percussion(int channel_number, short midi_number,
                               const Rational &beats,
                               const Rational &velocity_ratio,
                               const Rational &tempo_ratio, int time_offset,
                               qsizetype chord_number, qsizetype item_number,
                               const QString &item_description);
  void play_notes(qsizetype chord_number, const Chord &chord,
                  qsizetype first_note_index, qsizetype number_of_notes);
  void play_percussions(qsizetype chord_number, const Chord &chord,
                        qsizetype first_percussion_number,
                        qsizetype number_of_percussions);
  void play_chords(qsizetype first_chord_number, qsizetype number_of_chords,
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
