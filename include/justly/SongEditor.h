#pragma once

#include <fluidsynth.h>          // for new_fluid_event, new_fluid_sequencer2
#include <fluidsynth/types.h>    // for fluid_audio_driver_t, fluid_event_t
#include <qabstractitemmodel.h>  // for QModelIndex (ptr only), QAbstractI...
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for WindowFlags
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <string>  // for string

#include "justly/Song.h"           // for Song
#include "justly/StartingField.h"  // for StartingField
#include "justly/TreeLevel.h"      // for TreeLevel

class ChordsModel;
class InstrumentEditor;
class QAbstractItemView;
class QDoubleSpinBox;
class QItemSelection;
class QUndoStack;
struct Chord;
struct Instrument;

const auto PERCENT = 100;
const auto SECONDS_PER_MINUTE = 60;
const auto NUMBER_OF_MIDI_CHANNELS = 16;

class QAction;
class QWidget;

class SongEditor : public QMainWindow {
  Q_OBJECT

  Song song;

  QDoubleSpinBox* starting_tempo_editor_pointer;
  QDoubleSpinBox* starting_volume_editor_pointer;
  QDoubleSpinBox* starting_key_editor_pointer;
  InstrumentEditor* starting_instrument_editor_pointer;

  QAction* insert_before_action_pointer;
  QAction* insert_after_action_pointer;
  QAction* insert_into_action_pointer;
  QAction* remove_action_pointer;
  QAction* copy_action_pointer;
  QAction* paste_before_action_pointer;
  QAction* paste_after_action_pointer;
  QAction* paste_into_action_pointer;
  QAction* save_action_pointer;
  QAction* play_action_pointer;

  QAbstractItemView* chords_view_pointer;

  QUndoStack* undo_stack_pointer;

  ChordsModel* chords_model_pointer;

  QString current_file;
  QString current_folder;

  TreeLevel copy_level;

  double current_key = song.starting_key;
  double current_volume = song.starting_volume;
  double current_tempo = song.starting_tempo;
  const Instrument* current_instrument_pointer =
      song.starting_instrument_pointer;

  double starting_time = 0;
  double current_time = 0;

  fluid_event_t* event_pointer = new_fluid_event();
  fluid_settings_t* settings_pointer = new_fluid_settings();
  fluid_synth_t* synth_pointer = nullptr;
  fluid_sequencer_t* sequencer_pointer = new_fluid_sequencer2(0);
  fluid_audio_driver_t* audio_driver_pointer = nullptr;
  fluid_seq_id_t sequencer_id = -1;
  int soundfont_id = -1;

  void initialize_player();

  void modulate(const Chord* chord_pointer);

  auto play_notes(const Chord* chord_pointer, int first_note_index = 0,
                  int number_of_notes = -1) const -> double;

  [[nodiscard]] auto beat_time() const -> double;

  void export_recording();
  void open();
  void save_as();

  void set_starting_instrument(int);
  void set_starting_value(StartingField, const QVariant&);

  void fix_selection(const QItemSelection&, const QItemSelection&);

  void paste(int, const QModelIndex&);

  void update_actions();

  void start_real_time();
  auto play_chords(int first_chord_index = 0, int number_of_chords = -1)
      -> double;

 public:
  ~SongEditor() override;

  // prevent moving and copying;
  SongEditor(const SongEditor&) = delete;
  auto operator=(const SongEditor&) -> SongEditor = delete;
  SongEditor(SongEditor&&) = delete;
  auto operator=(SongEditor&&) -> SongEditor = delete;

  [[nodiscard]] auto get_chords_model_pointer() const -> QAbstractItemModel*;

  explicit SongEditor(QWidget* = nullptr, Qt::WindowFlags = Qt::WindowFlags());

  void open_file(const QString&);
  void save_as_file(const QString&);

  void paste_text(int, const std::string&, const QModelIndex&);

  void copy_selected();
  void insert_before();
  void insert_after();
  void insert_into();

  void paste_before();
  void paste_after();
  void paste_into();

  void remove_selected();
  void play_selected();

  void save();

  void undo();
  void redo();

  [[nodiscard]] auto get_current_file() const -> const QString&;

  void initialize_controls();

  void set_starting_control(StartingField value_type, const QVariant& new_value,
                            bool no_signals = false);
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;
  [[nodiscard]] auto starting_value(StartingField value_type) const -> QVariant;

  [[nodiscard]] auto get_number_of_children(int chord_number) const -> int;
  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
  void export_to(const std::string& output_file);

  void stop_playing();
};
