#pragma once

#include <qabstractitemmodel.h>   // for QModelIndex (ptr only), QAbstractI...
#include <qabstractitemview.h>    // for QAbstractItemView
#include <qitemselectionmodel.h>  // for QItemSelection (ptr only), QItemSe...
#include <qmainwindow.h>          // for QMainWindow
#include <qnamespace.h>           // for WindowFlags
#include <qspinbox.h>             // for QDoubleSpinBox
#include <qstring.h>              // for QString
#include <qtmetamacros.h>         // for Q_OBJECT
#include <qundostack.h>           // for QUndoStack
#include <qvariant.h>             // for QVariant

#include <memory>  // for unique_ptr
#include <string>  // for string

#include "justly/ChordsModel.h"       // for ChordsModel
#include "justly/Instrument.h"        // for Instrument
#include "justly/InstrumentEditor.h"  // for InstrumentEditor
#include "justly/Player.h"            // for Player
#include "justly/Song.h"              // for Song
#include "justly/StartingField.h"  // for StartingField, starting_instrument_id
#include "justly/TreeLevel.h"      // for TreeLevel

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

  std::unique_ptr<Player> player_pointer;

  TreeLevel copy_level;

  void export_recording();
  void open();
  void save_as();

  void set_starting_instrument(int);
  void set_starting_value(StartingField, const QVariant&);

  void initialize_controls();

  void fix_selection(const QItemSelection&, const QItemSelection&);

  void paste(int, const QModelIndex&);

  void update_actions();

 public:
  inline ~SongEditor() override { undo_stack_pointer->disconnect(); }

  // prevent moving and copying;
  SongEditor(const SongEditor&) = delete;
  auto operator=(const SongEditor&) -> SongEditor = delete;
  SongEditor(SongEditor&&) = delete;
  auto operator=(SongEditor&&) -> SongEditor = delete;

  [[nodiscard]] auto get_chords_model_pointer() const -> QAbstractItemModel*;

  explicit SongEditor(QWidget* = nullptr, Qt::WindowFlags = Qt::WindowFlags());

  inline void export_to(const QString& filename) {
    player_pointer->export_to(filename.toStdString());
  }

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
  void play_selected() const;

  void save();

  inline void play(int first_child_number, int number_of_children,
                   const QModelIndex& parent_index) const {
    player_pointer->play_selection(
        first_child_number, number_of_children,
        chords_model_pointer->get_chord_number(parent_index));
  }

  void stop() const;

  void undo();
  void redo();

  [[nodiscard]] inline auto get_current_file() const -> const QString& {
    return current_file;
  }

  inline void set_starting_control(StartingField value_type,
                                   const QVariant& new_value,
                                   bool no_signals = false) {
    switch (value_type) {
      case starting_key_id: {
        auto new_double = new_value.toDouble();
        if (starting_key_editor_pointer->value() != new_double) {
          if (no_signals) {
            starting_key_editor_pointer->blockSignals(true);
          }
          starting_key_editor_pointer->setValue(new_double);
          if (no_signals) {
            starting_key_editor_pointer->blockSignals(false);
          }
        }
        song.starting_key = new_double;
        break;
      }
      case starting_volume_id: {
        auto new_double = new_value.toDouble();
        if (starting_volume_editor_pointer->value() != new_double) {
          if (no_signals) {
            starting_volume_editor_pointer->blockSignals(true);
          }
          starting_volume_editor_pointer->setValue(new_double);
          if (no_signals) {
            starting_volume_editor_pointer->blockSignals(false);
          }
        }
        song.starting_volume = new_double;
        break;
      }
      case starting_tempo_id: {
        auto new_double = new_value.toDouble();
        if (starting_tempo_editor_pointer->value() != new_double) {
          if (no_signals) {
            starting_tempo_editor_pointer->blockSignals(true);
          }
          starting_tempo_editor_pointer->setValue(new_double);
          if (no_signals) {
            starting_tempo_editor_pointer->blockSignals(false);
          }
        }
        song.starting_tempo = new_double;
        break;
      }
      case starting_instrument_id:
        if (starting_instrument_editor_pointer->get_instrument_pointer() !=
            new_value.value<const Instrument*>()) {
          if (no_signals) {
            starting_instrument_editor_pointer->blockSignals(true);
          }
          starting_instrument_editor_pointer->set_instrument_pointer(
              new_value.value<const Instrument*>());
          if (no_signals) {
            starting_instrument_editor_pointer->blockSignals(false);
          }
        }
        song.starting_instrument_pointer = new_value.value<const Instrument*>();
        break;
      default:
        break;
    }
  }

  [[nodiscard]] inline auto get_selected_rows() const -> QModelIndexList {
    return chords_view_pointer->selectionModel()->selectedRows();
  }

  [[nodiscard]] inline auto get_starting_value(StartingField value_type) const
      -> QVariant {
    switch (value_type) {
      case starting_key_id:
        return QVariant::fromValue(song.starting_key);
      case starting_volume_id:
        return QVariant::fromValue(song.starting_volume);
      case starting_tempo_id:
        return QVariant::fromValue(song.starting_tempo);
      case starting_instrument_id:
        return QVariant::fromValue(song.starting_instrument_pointer);
      default:
        return {};
    }
  }

  [[nodiscard]] auto get_number_of_children(int chord_number) const -> int;
  [[nodiscard]] auto get_chords_view_pointer() const -> QAbstractItemView*;
};
