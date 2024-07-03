#pragma once

#include <fluidsynth.h>        // for new_fluid_event, new_fluid_se...
#include <fluidsynth/types.h>  // for fluid_audio_driver_t, fluid_e...
#include <qmainwindow.h>       // for QMainWindow
#include <qnamespace.h>        // for WindowFlags
#include <qtmetamacros.h>      // for Q_OBJECT

#include <cstddef>                // for size_t
#include <string>                 // for string
#include <vector>                 // for vector

#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT, NO_MOVE_COPY

class InstrumentEditor;
class QAction;
class QDoubleSpinBox;
class QSlider;
class QUndoStack;
class QWidget;
struct Chord;
struct ChordsView;
struct Instrument;

[[nodiscard]] auto JUSTLY_EXPORT get_default_driver() -> std::string;

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT
 public:
  std::string current_file;
  std::string current_folder;

  QSlider* const playback_volume_editor_pointer;
  InstrumentEditor* const starting_instrument_editor_pointer;
  QDoubleSpinBox* const starting_key_editor_pointer;
  QDoubleSpinBox* const starting_volume_editor_pointer;
  QDoubleSpinBox* const starting_tempo_editor_pointer;

  ChordsView* const chords_view_pointer;

  QUndoStack* const undo_stack_pointer;

  QAction* const insert_before_action_pointer;
  QAction* const insert_after_action_pointer;
  QAction* const insert_into_action_pointer;
  QAction* const remove_action_pointer;

  QAction* const copy_action_pointer;
  QAction* const paste_cell_action_pointer;
  QAction* const paste_before_action_pointer;
  QAction* const paste_after_action_pointer;
  QAction* const paste_into_action_pointer;

  QAction* const save_action_pointer;
  QAction* const play_action_pointer;
  QAction* const stop_playing_action_pointer;

  std::vector<unsigned int> channel_schedules;

  double starting_time = 0;
  double current_time = 0;

  double current_key = 0;
  double current_volume = 0;
  double current_tempo = 0;
  const Instrument* current_instrument_pointer = nullptr;

  fluid_settings_t* settings_pointer = new_fluid_settings();
  unsigned int soundfont_id = 0;
  fluid_synth_t* synth_pointer = nullptr;
  fluid_event_t* event_pointer = new_fluid_event();
  fluid_sequencer_t* sequencer_pointer = new_fluid_sequencer2(0);
  fluid_seq_id_t sequencer_id = -1;
  fluid_audio_driver_t* audio_driver_pointer = nullptr;

  void update_actions() const;

  explicit SongEditor(QWidget* parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  NO_MOVE_COPY(SongEditor)
  ~SongEditor() override;

  void open_file(const std::string& filename);
  void save();
  void save_as_file(const std::string& filename);

  [[nodiscard]] auto beat_time() const -> double;
  void initialize_play();
  [[nodiscard]] auto has_real_time() const -> bool;
  void modulate(const Chord& chord);
  auto play_notes(size_t chord_index, const Chord& chord,
                  size_t first_note_index, size_t number_of_notes)
      -> unsigned int;
  auto play_chords(size_t first_chord_index, size_t number_of_chords,
                   int wait_frames = 0) -> unsigned int;

  void start_real_time(const std::string& driver = get_default_driver());

  [[nodiscard]] auto get_playback_volume() const -> float;
  void set_playback_volume(float value) const;

  void stop_playing() const;

  void export_to_file(const std::string& output_file);
};

