#pragma once

#include <fluidsynth.h>          // for new_fluid_event, new_fluid_sequen...
#include <fluidsynth/types.h>    // for fluid_audio_driver_t, fluid_event_t
#include <qabstractitemmodel.h>  // for QAbstractItemModel (ptr only)
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for WindowFlags
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT

#include <cstddef>  // for size_t
#include <string>   // for string

#include "justly/NoteChordField.h"  // for symbol_column, NoteChordField
#include "justly/Song.h"            // for Song
#include "justly/TreeLevel.h"       // for TreeLevel
#include "justly/global.h"

class ChordsModel;
class InstrumentEditor;
class QAbstractItemView;
class QAction;
class QDoubleSpinBox;
class QItemSelection;
class QUndoStack;
class QWidget;
struct Chord;
struct Instrument;

const auto PERCENT = 100;
const auto SECONDS_PER_MINUTE = 60;
const auto NUMBER_OF_MIDI_CHANNELS = 16;

class JUSTLY_EXPORT SongEditor : public QMainWindow {
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

  unsigned int starting_time = 0;
  unsigned int current_time = 0;

  fluid_event_t* event_pointer = new_fluid_event();
  fluid_settings_t* settings_pointer = new_fluid_settings();
  fluid_synth_t* synth_pointer = nullptr;
  fluid_sequencer_t* sequencer_pointer = new_fluid_sequencer2(0);
  fluid_audio_driver_t* audio_driver_pointer = nullptr;
  fluid_seq_id_t sequencer_id = -1;
  unsigned int soundfont_id = 0;

  void initialize_player();

  void modulate(const Chord* chord_pointer);

  auto play_notes(const Chord* chord_pointer, size_t first_note_index,
                  size_t number_of_notes) const -> unsigned int;
  auto play_all_notes(const Chord* chord_pointer,
                      size_t first_note_index = 0) const -> unsigned int;

  [[nodiscard]] auto beat_time() const -> double;

  void export_recording();
  void open();
  void save_as();

  void fix_selection(const QItemSelection&, const QItemSelection&);

  void paste(int, const QModelIndex&);

  void update_actions();

  void start_real_time();
  auto play_chords(size_t first_chord_index,
                   size_t number_of_chords) -> unsigned int;
  auto play_all_chords(size_t first_chord_index = 0) -> unsigned int;
  void initialize_controls();

 public:
  ~SongEditor() override;

  // prevent moving and copying;
  SongEditor(const SongEditor&) = delete;
  auto operator=(const SongEditor&) -> SongEditor = delete;
  SongEditor(SongEditor&&) = delete;
  auto operator=(SongEditor&&) -> SongEditor = delete;

  [[nodiscard]] auto get_chords_model_pointer() const -> QAbstractItemModel*;
  [[nodiscard]] auto get_song_pointer() const -> const Song*;

  [[nodiscard]] auto get_index(
      int = -1, int = -1, NoteChordField = symbol_column) const -> QModelIndex;

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

  void set_starting_instrument_undoable(const Instrument* new_value);
  void set_starting_key_undoable(double new_value);
  void set_starting_tempo_undoable(double new_value);
  void set_starting_volume_undoable(double new_value);

  void set_starting_instrument(const Instrument* new_value);
  void set_starting_key(double new_value);
  void set_starting_tempo(double new_value);
  void set_starting_volume(double new_value);

  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
  void export_to(const std::string& output_file);

  void stop_playing();

  void select_index(QModelIndex);
  void select_indices(QModelIndex, QModelIndex);
  void clear_selection();
};
