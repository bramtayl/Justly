#pragma once

#include <QMainWindow>
#include <QString>
#include <Qt>
#include <cstddef>
#include <fluidsynth.h>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

struct ChordsModel;
struct ChordsView;
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

[[nodiscard]] auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;

struct SongEditor : public QMainWindow {
  QDoubleSpinBox *const gain_editor_pointer;
  QDoubleSpinBox *const starting_key_editor_pointer;
  QDoubleSpinBox *const starting_velocity_editor_pointer;
  QDoubleSpinBox *const starting_tempo_editor_pointer;

  QString current_file;

  QUndoStack *const undo_stack_pointer;

  ChordsView *const chords_view_pointer;
  ChordsModel *const chords_model_pointer;

  QAction *const insert_after_action_pointer;
  QAction *const insert_into_action_pointer;
  QAction *const delete_action_pointer;

  QAction *const cut_action_pointer;
  QAction *const copy_action_pointer;
  QAction *const paste_action_pointer;

  QAction *const save_action_pointer;
  QAction *const play_action_pointer;
  QAction *const stop_playing_action_pointer;

  QAction *const expand_action_pointer;
  QAction *const collapse_action_pointer;

  QString current_folder;

  double starting_time = 0;
  double current_time = 0;
  double final_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;

  std::vector<double> channel_schedules;

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
void play_chords(SongEditor *song_editor_pointer, size_t first_chord_number,
                 size_t number_of_chords, int wait_frames = 0);
void stop_playing(fluid_sequencer_t *sequencer_pointer,
                  fluid_event_t *event_pointer);
void delete_audio_driver(SongEditor *song_editor_pointer);
