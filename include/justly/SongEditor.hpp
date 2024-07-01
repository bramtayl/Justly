#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QModelIndexList
#include <qmainwindow.h>         // for QMainWindow
#include <qnamespace.h>          // for ItemDataRole, WindowFlags
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <string>   // for string

#include "justly/CopyType.hpp"
#include "justly/NoteChordField.hpp"    // for NoteChordField, symbol_column
#include "justly/Player.hpp"            // for Player
#include "justly/Song.hpp"              // for Song
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
struct Instrument;

class JUSTLY_EXPORT SongEditor : public QMainWindow {
  Q_OBJECT

  Song song;
  Player player = Player(&song);

  CopyType copy_type;

  std::string current_file;
  std::string current_folder;

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
  QAction* paste_cell_action_pointer;
  QAction* paste_before_action_pointer;
  QAction* paste_after_action_pointer;
  QAction* paste_into_action_pointer;

  QAction* save_action_pointer;
  QAction* play_action_pointer;

  void update_actions();

  void paste_cell();
  void paste_rows(int first_child_number, const QModelIndex& parent_index);

  void open();
  void save_as();
  void export_wav();

  void initialize_controls();

 public:
  explicit SongEditor(QWidget* parent_pointer = nullptr,
                      Qt::WindowFlags flags = Qt::WindowFlags());
  NO_MOVE_COPY(SongEditor)
  ~SongEditor() override;

  [[nodiscard]] auto get_current_file() const -> const std::string&;
  [[nodiscard]] auto get_selected_rows() const -> QModelIndexList;

  [[nodiscard]] auto get_index(
      int parent_number, int child_number,
      NoteChordField column_number = symbol_column) const -> QModelIndex;

  void select_index(QModelIndex index);
  void select_indices(QModelIndex first_index, QModelIndex last_index);
  void clear_selection();

  [[nodiscard]] auto get_number_of_children(int parent_index) -> size_t;

  [[nodiscard]] auto get_header_data(NoteChordField column_number,
                                     Qt::Orientation orientation,
                                     Qt::ItemDataRole role) const -> QVariant;
  [[nodiscard]] auto get_row_count(QModelIndex index) const -> int;
  [[nodiscard]] auto get_column_count(QModelIndex index) const -> int;
  [[nodiscard]] auto get_parent_index(QModelIndex index) const -> QModelIndex;
  [[nodiscard]] auto get_flags(QModelIndex index) const -> Qt::ItemFlags;
  [[nodiscard]] auto get_data(QModelIndex index, Qt::ItemDataRole role)
      -> QVariant;
  [[nodiscard]] auto set_data(QModelIndex index, const QVariant& new_value,
                              Qt::ItemDataRole role) -> bool;

  [[nodiscard]] auto create_editor(QModelIndex index) -> QWidget*;
  void set_editor(QWidget* cell_editor_pointer, QModelIndex index,
                  const QVariant& new_value);

  [[nodiscard]] auto get_playback_volume() const -> float;
  void set_playback_volume(float new_playback_volume);

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
  void paste_cell_text(const QModelIndex& cell_index, const std::string& text);
  void paste_rows_text(int first_child_number, const std::string& text,
                  const QModelIndex& parent_index);
  void paste_before();
  void paste_after();
  void paste_into();

  void insert_before();
  void insert_after();
  void insert_into();
  void remove_selected();

  void open_file(const std::string& filename);
  void save();
  void save_as_file(const std::string& filename);
  void export_to_file(const std::string& output_file);

  void start_real_time();
  void play_selected();
  void stop_playing();
};
