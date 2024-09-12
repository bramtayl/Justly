#pragma once

#include <QList>
#include <QMainWindow>
#include <QString>
#include <Qt>
#include <QtGlobal>
#include <fluidsynth.h>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

struct ChordsModel;
struct JustlyView;
struct NotesModel;
struct PercussionsModel;
class QAction;
class QCloseEvent;
class QDoubleSpinBox;
class QUndoStack;
class QWidget;

const auto MAX_GAIN = 10;
const auto MAX_VELOCITY = 127;
const auto MIN_STARTING_KEY = 60;
const auto MAX_STARTING_KEY = 440;
const auto MIN_STARTING_TEMPO = 25;
const auto MAX_STARTING_TEMPO = 200;

enum ModelType {
  chords_type,
  notes_type,
  percussion_type
};

[[nodiscard]] auto make_validator(const char* title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;

struct SongEditor : public QMainWindow {
  Q_OBJECT;
 public:
  QDoubleSpinBox *const gain_editor_pointer;
  QDoubleSpinBox *const starting_key_editor_pointer;
  QDoubleSpinBox *const starting_velocity_editor_pointer;
  QDoubleSpinBox *const starting_tempo_editor_pointer;

  QString current_file;

  QUndoStack *const undo_stack_pointer;

  JustlyView *const table_view_pointer;
  ChordsModel *const chords_model_pointer;
  NotesModel* const notes_model_pointer;
  PercussionsModel* const percussions_model_pointer;

  QAction *const edit_chords_action_pointer;
  QAction *const edit_notes_action_pointer;
  QAction *const edit_percussions_action_pointer;
  
  int current_chord_number = -1;
  ModelType current_model_type = chords_type;

  QAction *const insert_after_action_pointer;
  QAction *const insert_into_action_pointer;
  QAction *const delete_action_pointer;
  QAction *const remove_rows_action_pointer;

  QAction *const cut_action_pointer;
  QAction *const copy_action_pointer;
  QAction *const paste_action_pointer;

  QAction *const save_action_pointer;
  QAction *const play_action_pointer;
  QAction *const stop_playing_action_pointer;

  QString current_folder;

  double starting_time = 0;
  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  QList<double> channel_schedules;

  fluid_settings_t *const settings_pointer;
  fluid_event_t *const event_pointer;
  fluid_sequencer_t *const sequencer_pointer;

  fluid_synth_t *synth_pointer;
  const unsigned int soundfont_id;
  const fluid_seq_id_t sequencer_id;
  fluid_audio_driver_t *audio_driver_pointer = nullptr;

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
  void play();
  void stop_playing() const;
  void open();
  void save();
  void save_as();
  void export_wav();
  void cut();
  void copy_selected() const;
  void insert_after() const;
  void paste_cells();
  void delete_selected();
  void remove_rows();
  void insert_into() const;
  void edit_chords();
  void edit_notes();
  void edit_percussions();
  void set_gain(double new_value);
  void set_starting_key(double new_value);
  void set_starting_velocity(double new_value);
  void set_starting_tempo(double new_value);
  void change_clean() const;
};

void set_gain_directly(const SongEditor *song_editor_pointer, double new_gain);
void set_starting_key_directly(const SongEditor *song_editor_pointer,
                               double new_value);
void set_starting_velocity_directly(const SongEditor *song_editor_pointer,
                                    double new_value);
void set_starting_tempo_directly(const SongEditor *song_editor_pointer,
                                 double new_value);

void start_real_time(SongEditor *song_editor_pointer);
void initialize_play(SongEditor *song_editor_pointer);
void send_event_at(fluid_sequencer_t *sequencer_pointer,
                   fluid_event_t *event_pointer, double time);
void play_chords(SongEditor *song_editor_pointer, qsizetype first_chord_number,
                 qsizetype number_of_chords, int wait_frames = 0);
void delete_audio_driver(SongEditor *song_editor_pointer);
