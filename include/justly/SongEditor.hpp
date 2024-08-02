#pragma once

#include <QAbstractItemModel>
#include <QMainWindow>
#include <QObject>
#include <QString>
#include <QVariant>
#include <Qt>
#include <cstddef>
#include <fluidsynth.h>
#include <vector>

#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/NoteChordColumn.hpp"

struct Chord;
struct ChordsView;
class Instrument;
class InstrumentEditor;
class QAction;
class QDoubleSpinBox;
class QItemSelectionModel;
class QSlider;
class QUndoStack;
class QWidget;

void JUSTLY_EXPORT register_converters();

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT
private:
  QString current_folder;

  double starting_time = 0;
  double current_time = 0;

  const Instrument *current_instrument_pointer;
  double current_key = 0;
  double current_volume = 0;
  double current_tempo = 0;

  std::vector<double> channel_schedules;

  fluid_settings_t *const settings_pointer;
  fluid_event_t *const event_pointer;
  fluid_sequencer_t *const sequencer_pointer;

  fluid_synth_t * synth_pointer;
  const int soundfont_id;
  const fluid_seq_id_t sequencer_id;
  fluid_audio_driver_t *audio_driver_pointer = nullptr;

  [[nodiscard]] auto beat_time() const -> double;

  void set_playback_volume(float value) const;
  void send_event_at(double time) const;

  void start_real_time();
  void initialize_play();
  void modulate(const Chord &chord);
  auto play_notes(size_t chord_index, const Chord &chord,
                  size_t first_note_index, size_t number_of_notes) -> double;
  auto play_chords(size_t first_chord_number, size_t number_of_chords,
                   int wait_frames = 0) -> double;
  void stop_playing() const;
  void delete_audio_driver();

  void update_actions() const;

public:
  const Instrument *starting_instrument_pointer;
  double starting_key;
  double starting_volume_percent;
  double starting_tempo;

  QSlider *const playback_volume_editor_pointer;

  InstrumentEditor *const starting_instrument_editor_pointer;
  QDoubleSpinBox *const starting_key_editor_pointer;
  QDoubleSpinBox *const starting_volume_percent_editor_pointer;
  QDoubleSpinBox *const starting_tempo_editor_pointer;

  QString current_file;

  QUndoStack *const undo_stack_pointer;

  ChordsView *const chords_view_pointer;

  QAction *const insert_after_action_pointer;
  QAction *const insert_into_action_pointer;
  QAction *const delete_action_pointer;

  QAction *const copy_action_pointer;
  QAction *const paste_cells_or_after_action_pointer;
  QAction *const paste_into_action_pointer;

  QAction *const save_action_pointer;
  QAction *const play_action_pointer;
  QAction *const stop_playing_action_pointer;

  explicit SongEditor(QWidget *parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  ~SongEditor() override;

  // prevent moving and copying
  SongEditor(const SongEditor &) = delete;
  auto operator=(const SongEditor &) -> SongEditor = delete;
  SongEditor(SongEditor &&) = delete;
  auto operator=(SongEditor &&) -> SongEditor = delete;

  [[nodiscard]] auto get_playback_volume() const -> float;

  [[nodiscard]] auto get_chord_index(
      size_t chord_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;
  [[nodiscard]] auto get_note_index(
      size_t chord_number, size_t note_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;

  [[nodiscard]] auto get_chords_model() const -> QAbstractItemModel*;
  [[nodiscard]] auto get_selection_model() const -> QItemSelectionModel*;

  void set_instrument(const Instrument *new_value) const;

  void set_instrument_directly(const Instrument *new_value);
  void set_key_directly(double new_value);
  void set_volume_directly(double new_value);
  void set_tempo_directly(double new_value);

  [[nodiscard]] auto create_editor(QModelIndex index) const -> QWidget *;
  void set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                  const QVariant &new_value) const;

  void open_file(const QString &filename);
  void save_as_file(const QString &filename);
  void export_to_file(const QString &output_file);
};
