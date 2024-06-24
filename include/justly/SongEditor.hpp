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

#include "justly/NoteChordField.hpp"  // for symbol_column, NoteChordField
#include "justly/Song.hpp"            // for Song
#include "justly/TreeLevel.hpp"       // for TreeLevel
#include "justly/public_constants.hpp"

class ChordsModel;
class InstrumentEditor;
class QAbstractItemView;
class QAction;
class QDoubleSpinBox;
class QItemSelection;
class QSlider;
class QUndoStack;
class QWidget;
struct Chord;
struct Instrument;

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT

  Song song;

  TreeLevel copy_level;

  QString current_file;
  QString current_folder;

  QSlider* master_volume_editor_pointer;
  InstrumentEditor* starting_instrument_editor_pointer;
  QDoubleSpinBox* starting_key_editor_pointer;
  QDoubleSpinBox* starting_volume_editor_pointer;
  QDoubleSpinBox* starting_tempo_editor_pointer;

  ChordsModel* chords_model_pointer = nullptr;
  QAbstractItemView* chords_view_pointer;

  QUndoStack* undo_stack_pointer;

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

  std::vector<unsigned int> channel_schedules;

  float master_volume;

  double starting_time = 0;
  double current_time = 0;

  double current_key = song.starting_key;
  double current_volume = song.starting_volume;
  double current_tempo = song.starting_tempo;
  const Instrument* current_instrument_pointer =
      song.starting_instrument_pointer;

  fluid_settings_t* settings_pointer = new_fluid_settings();
  unsigned int soundfont_id = 0;
  fluid_synth_t* synth_pointer = nullptr;
  fluid_event_t* event_pointer = new_fluid_event();
  fluid_sequencer_t* sequencer_pointer = new_fluid_sequencer2(0);
  fluid_seq_id_t sequencer_id = -1;
  fluid_audio_driver_t* audio_driver_pointer = nullptr;

  void fix_selection(const QItemSelection& selected,
                     const QItemSelection& /*deselected*/);
  void update_actions();

  void paste(int first_child_number, const QModelIndex& parent_index);

  void open();
  void save_as();

  void export_recording();

  void initialize_controls();

  [[nodiscard]] auto beat_time() const -> double;

  void start_real_time();
  void initialize_play();

  void modulate(const Chord* chord_pointer);

  auto play_notes(int chord_index, const Chord* chord_pointer, size_t first_note_index,
                  size_t number_of_notes) -> unsigned int;
  auto play_all_notes(int chord_index, const Chord* chord_pointer, size_t first_note_index = 0)
      -> unsigned int;

  auto play_chords(size_t first_chord_index, size_t number_of_chords)
      -> unsigned int;
  auto play_all_chords(size_t first_chord_index = 0) -> unsigned int;

 public:
  explicit SongEditor(QWidget* parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  NO_MOVE_COPY(SongEditor)
  ~SongEditor() override;

  [[nodiscard]] auto get_song_pointer() const -> const Song*;
  [[nodiscard]] auto get_current_file() const -> const QString&;
  [[nodiscard]] auto get_chords_model_pointer() const -> QAbstractItemModel*;
  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
  [[nodiscard]] auto get_master_volume() const -> double;
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_index(int chord_number = -1, int note_number = -1,
                               NoteChordField = symbol_column) const
      -> QModelIndex;
  void select_index(QModelIndex index);
  void select_indices(QModelIndex first_index, QModelIndex last_index);
  void clear_selection();

  void set_master_volume(double new_master_volume);

  void set_starting_instrument(const Instrument* new_value);
  void set_starting_instrument_undoable(const Instrument* new_value);

  void set_starting_key(double new_value);
  void set_starting_key_undoable(double new_value);

  void set_starting_volume(double new_value);
  void set_starting_volume_undoable(double new_value);

  void set_starting_tempo(double new_value);
  void set_starting_tempo_undoable(double new_value);

  void undo();
  void redo();

  void copy_selected();
  void paste_text(int first_child_number, const std::string& text,
                  const QModelIndex& parent_index);
  void paste_before();
  void paste_after();
  void paste_into();

  void insert_before();
  void insert_after();
  void insert_into();
  void remove_selected();

  void open_file(const QString& filename);
  void save();
  void save_as_file(const QString& filename);
  void export_to(const std::string& output_file);

  void play_selected();
  void stop_playing();
};

// TODO: remove get_song_pointer, get_chords_model_pointer, and get_chords_view_pointer
// TODO: move action functions into lambda/private functions and change public f

auto get_editor_data(QWidget* cell_editor_pointer, int column) -> QVariant;
void set_editor_data(QWidget* cell_editor_pointer, int column, const QVariant& new_value);