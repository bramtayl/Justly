#pragma once

#include <fluidsynth.h>  // for new_fluid_event, new_fluid_se...

#include <QAction>
#include <QMainWindow>  // for QMainWindow
#include <QObject>      // for Q_OBJECT
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QString>  // for QString
#include <QUndoStack>
#include <QWidget>
#include <Qt>       // for WindowFlags
#include <cstddef>  // for size_t
#include <string>   // for string
#include <vector>   // for vector

#include "justly/ChangeId.hpp"
#include "justly/Chord.hpp"
#include "justly/ChordsView.hpp"
#include "justly/Instrument.hpp"
#include "justly/InstrumentEditor.hpp"
#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT

const auto NUMBER_OF_MIDI_CHANNELS = 16;

[[nodiscard]] auto get_default_driver() -> std::string;
auto get_settings_pointer() -> fluid_settings_t*;
auto get_soundfont_id(fluid_synth_t* synth_pointer) -> int;

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT
 private:
  std::string current_folder =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
          .toStdString();

  double starting_time = 0;
  double current_time = 0;

  const Instrument* current_instrument_pointer = get_instrument_pointer("");
  double current_key = 0;
  double current_volume = 0;
  double current_tempo = 0;

  std::vector<unsigned int> channel_schedules =
      std::vector<unsigned int>(NUMBER_OF_MIDI_CHANNELS, 0);

  fluid_settings_t* const settings_pointer = get_settings_pointer();
  fluid_event_t* const event_pointer = new_fluid_event();
  fluid_sequencer_t* const sequencer_pointer = new_fluid_sequencer2(0);

  fluid_synth_t* const synth_pointer = new_fluid_synth(settings_pointer);
  const unsigned int soundfont_id = get_soundfont_id(synth_pointer);
  const fluid_seq_id_t sequencer_id =
      fluid_sequencer_register_fluidsynth(sequencer_pointer, synth_pointer);
  fluid_audio_driver_t* audio_driver_pointer = nullptr;

  [[nodiscard]] auto beat_time() const -> double;
  [[nodiscard]] auto has_real_time() const -> bool;
  void set_playback_volume(float value) const;

  void start_real_time(const std::string& driver = get_default_driver());
  void initialize_play();
  void modulate(const Chord& chord);
  auto play_notes(size_t chord_index, const Chord& chord,
                  size_t first_note_index, size_t number_of_notes)
      -> unsigned int;
  auto play_chords(size_t first_chord_index, size_t number_of_chords,
                   int wait_frames = 0) -> unsigned int;
  void stop_playing() const;

  void update_actions() const;

 public:
  const Instrument* starting_instrument_pointer;
  double starting_key;
  double starting_volume_percent;
  double starting_tempo;

  QSlider* const playback_volume_editor_pointer =
      new QSlider(Qt::Horizontal, this);

  InstrumentEditor* const starting_instrument_editor_pointer =
      new InstrumentEditor(this, false);
  QDoubleSpinBox* const starting_key_editor_pointer = new QDoubleSpinBox(this);
  QDoubleSpinBox* const starting_volume_editor_pointer =
      new QDoubleSpinBox(this);
  QDoubleSpinBox* const starting_tempo_editor_pointer =
      new QDoubleSpinBox(this);

  std::string current_file;

  QUndoStack* const undo_stack_pointer = new QUndoStack(this);

  ChordsView* const chords_view_pointer =
      new ChordsView(undo_stack_pointer, this);

  QAction* const insert_before_action_pointer =
      new QAction(tr("&Before"), this);
  QAction* const insert_after_action_pointer = new QAction(tr("&After"), this);
  QAction* const insert_into_action_pointer = new QAction(tr("&Into"), this);
  QAction* const remove_action_pointer = new QAction(tr("&Remove"), this);

  QAction* const copy_action_pointer = new QAction(tr("&Copy"), this);
  QAction* const paste_cell_action_pointer = new QAction(tr("&Cell"), this);
  QAction* const paste_before_action_pointer = new QAction(tr("&Before"), this);
  QAction* const paste_after_action_pointer = new QAction(tr("&After"), this);
  QAction* const paste_into_action_pointer = new QAction(tr("&Into"), this);

  QAction* const save_action_pointer = new QAction(tr("&Save"), this);
  QAction* const play_action_pointer = new QAction(tr("&Play selection"), this);
  QAction* const stop_playing_action_pointer =
      new QAction(tr("&Stop playing"), this);

  explicit SongEditor(QWidget* parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  ~SongEditor() override;

  // prevent moving and copying
  SongEditor(const SongEditor&) = delete;
  auto operator=(const SongEditor&) -> SongEditor = delete;
  SongEditor(SongEditor&&) = delete;
  auto operator=(SongEditor&&) -> SongEditor = delete;

  [[nodiscard]] auto get_playback_volume() const -> float;

  void set_instrument_directly(const Instrument* new_value);
  void set_double_directly(ChangeId change_id, double new_value);

  void open_file(const std::string& filename);
  void save_as_file(const std::string& filename);
  void export_to_file(const std::string& output_file);
};
