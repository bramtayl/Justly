#pragma once

#include <fluidsynth.h>          // for new_fluid_event, new_fluid_se...
#include <fluidsynth/types.h>    // for fluid_audio_driver_t, fluid_e...
#include <qabstractitemmodel.h>  // for QModelIndex, QModelIndexList
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for ItemDataRole, WindowFlags
#include <qstring.h>             // for QString
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <string>   // for string
#include <vector>   // for vector

#include "justly/NoteChordField.hpp"    // for symbol_column, NoteChordField
#include "justly/Song.hpp"              // for Song
#include "justly/TreeLevel.hpp"         // for TreeLevel
#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT, NO_MOVE_COPY

class ChordsModel;
class InstrumentEditor;
class QAbstractItemView;
class QAction;
class QDoubleSpinBox;
class QSlider;
class QUndoStack;
class QWidget;
class TreeSelector;
struct Chord;
struct Instrument;

auto get_default_driver() -> std::string;

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT

  Song song;

  TreeLevel copy_level;

  QString current_file;
  QString current_folder;

  QSlider* playback_volume_editor_pointer;
  InstrumentEditor* starting_instrument_editor_pointer;
  QDoubleSpinBox* starting_key_editor_pointer;
  QDoubleSpinBox* starting_volume_editor_pointer;
  QDoubleSpinBox* starting_tempo_editor_pointer;

  TreeSelector* tree_selector_pointer;

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

  float playback_volume;

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

  void update_actions();

  void paste(int first_child_number, const QModelIndex& parent_index);

  void open();
  void save_as();

  void export_recording();

  void initialize_controls();

  [[nodiscard]] auto beat_time() const -> double;

  void initialize_play();

  void modulate(const Chord* chord_pointer);

  auto play_notes(size_t chord_index, const Chord* chord_pointer,
                  size_t first_note_index, size_t number_of_notes)
      -> unsigned int;

  auto play_chords(size_t first_chord_index, size_t number_of_chords)
      -> unsigned int;

 public:
  explicit SongEditor(QWidget* parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  NO_MOVE_COPY(SongEditor)
  ~SongEditor() override;

  [[nodiscard]] auto get_current_file() const -> const QString&;
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_index(
      int chord_number = -1, int note_number = -1,
      NoteChordField column_number = symbol_column) const -> QModelIndex;

  void select_index(QModelIndex index);
  void select_indices(QModelIndex first_index, QModelIndex last_index);
  void clear_selection();

  [[nodiscard]] auto get_number_of_children(int parent_index) -> size_t;

  [[nodiscard]] auto get_header_data(int column_number,
                                     Qt::Orientation orientation,
                                     Qt::ItemDataRole role) const -> QVariant;
  [[nodiscard]] auto get_row_count(QModelIndex index) const -> int;
  [[nodiscard]] auto get_column_count(QModelIndex index) const -> int;
  [[nodiscard]] auto get_parent_index(QModelIndex index) const -> QModelIndex;
  [[nodiscard]] auto size_hint_for_column(int column) const -> int;
  [[nodiscard]] auto get_flags(QModelIndex index) const -> Qt::ItemFlags;
  [[nodiscard]] auto get_data(QModelIndex index, Qt::ItemDataRole role)
      -> QVariant;
  [[nodiscard]] auto set_data(QModelIndex index, const QVariant& new_value,
                              Qt::ItemDataRole role) -> bool;

  [[nodiscard]] auto create_editor(QModelIndex index) -> QWidget*;
  void set_editor(QWidget* cell_editor_pointer, QModelIndex index,
                  const QVariant& new_value);

  [[nodiscard]] auto get_playback_volume() const -> double;
  void set_playback_volume(double new_playback_volume);

  [[nodiscard]] auto get_starting_instrument() const -> const Instrument*;
  void set_starting_instrument_directly(const Instrument* new_value);
  void set_starting_instrument_undoable(const Instrument* new_value);

  [[nodiscard]] auto get_starting_key() const -> double;
  void set_starting_key_directly(double new_value);
  void set_starting_key_undoable(double new_value);

  [[nodiscard]] auto get_starting_volume() const -> double;
  void set_starting_volume_directly(double new_value);
  void set_starting_volume_undoable(double new_value);

  [[nodiscard]] auto get_starting_tempo() const -> double;
  void set_starting_tempo_directly(double new_value);
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

  void start_real_time(const std::string& driver = get_default_driver());

  void play_selected();
  void stop_playing();
};
