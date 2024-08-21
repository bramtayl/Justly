#pragma once

#include <QAbstractItemModel>
#include <QFileDialog>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QVariant>
#include <Qt>
#include <cstddef>
#include <fluidsynth.h>
#include <string>
#include <vector>

#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/NoteChordColumn.hpp"

struct Chord;
struct ChordsModel;
struct ChordsView;
struct Instrument;
class InstrumentEditor;
class QAction;
class QCloseEvent;
class QDoubleSpinBox;
class QTreeView;
class QUndoStack;
class QWidget;

void JUSTLY_EXPORT register_converters();

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT
private:
  QDoubleSpinBox *const gain_editor_pointer;
  InstrumentEditor *const starting_instrument_editor_pointer;
  QDoubleSpinBox *const starting_key_editor_pointer;
  QDoubleSpinBox *const starting_velocity_editor_pointer;
  QDoubleSpinBox *const starting_tempo_editor_pointer;

  QString current_file;

  QUndoStack *const undo_stack_pointer;

  ChordsView *const chords_view_pointer;
  ChordsModel *const chords_model_pointer;

  QAction *const toggle_percussion_action_pointer;

  QAction *const insert_after_action_pointer;
  QAction *const insert_into_action_pointer;
  QAction *const delete_action_pointer;

  QAction *const cut_action_pointer;
  QAction *const copy_action_pointer;
  QAction *const paste_cells_or_after_action_pointer;
  QAction *const paste_into_action_pointer;

  QAction *const save_action_pointer;
  QAction *const play_action_pointer;
  QAction *const stop_playing_action_pointer;

  QAction *const expand_action_pointer;
  QAction *const collapse_action_pointer;

  QString current_folder;

  double starting_time = 0;
  double current_time = 0;
  double final_time = 0;

  const Instrument *current_instrument_pointer;
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

  [[nodiscard]] auto get_selected_file(QFileDialog *dialog_pointer) -> QString;

  void start_real_time();
  void initialize_play();
  void modulate(const Chord &chord);
  void update_final_time(double new_final_time);
  void play_notes(size_t chord_index, const Chord &chord,
                  size_t first_note_index, size_t number_of_notes);
  void play_chords(size_t first_chord_number, size_t number_of_chords,
                   int wait_frames = 0);
  void delete_audio_driver();

  void update_actions() const;

public:
  explicit SongEditor(QWidget *parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  ~SongEditor() override;

  // prevent moving and copying
  SongEditor(const SongEditor &) = delete;
  auto operator=(const SongEditor &) -> SongEditor = delete;
  SongEditor(SongEditor &&) = delete;
  auto operator=(SongEditor &&) -> SongEditor = delete;

  void closeEvent(QCloseEvent *close_event_pointer) override;

  [[nodiscard]] auto get_chord_index(
      size_t chord_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;
  [[nodiscard]] auto get_note_index(
      size_t chord_number, size_t note_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;

  [[nodiscard]] auto get_chords_view_pointer() const -> QTreeView *;

  [[nodiscard]] auto get_gain() const -> double;
  [[nodiscard]] auto get_starting_instrument_name() const -> std::string;
  [[nodiscard]] auto get_starting_key() const -> double;
  [[nodiscard]] auto get_starting_velocity() const -> double;
  [[nodiscard]] auto get_starting_tempo() const -> double;

  [[nodiscard]] auto get_current_file() const -> QString;

  void set_gain(double new_value) const;
  void set_starting_instrument_name(const std::string &new_name) const;
  void set_starting_key(double new_value) const;
  void set_starting_velocity(double new_value) const;
  void set_starting_tempo(double new_value) const;

  void set_gain_directly(double new_gain);
  void set_starting_instrument_directly(const Instrument *new_value);
  void set_starting_key_directly(double new_value);
  void set_starting_velocity_directly(double new_value);
  void set_starting_tempo_directly(double new_value);

  [[nodiscard]] auto create_editor(QModelIndex index) const -> QWidget *;
  void set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                  const QVariant &new_value) const;

  void undo() const;
  void redo() const;

  void trigger_insert_after() const;
  void trigger_insert_into() const;
  void trigger_delete() const;

  void trigger_cut() const;
  void trigger_copy() const;
  void trigger_paste_cells_or_after() const;
  void trigger_paste_into() const;

  void trigger_save() const;

  void trigger_play() const;
  void trigger_stop_playing() const;

  void trigger_expand() const;
  void trigger_collapse() const;

  [[nodiscard]] auto
  make_file_dialog(const QString &caption, const QString &filter,
                   QFileDialog::AcceptMode accept_mode, const QString &suffix,
                   QFileDialog::FileMode file_mode) -> QFileDialog *;

  void open_file(const QString &filename);
  void save_as_file(const QString &filename);
  void export_to_file(const QString &output_file);
};
