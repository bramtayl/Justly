#pragma once

#include <fluidsynth.h>           // for fluid_event_all_sounds_off, flu...
#include <fluidsynth/types.h>     // for fluid_audio_driver_t, fluid_eve...
#include <qabstractitemmodel.h>   // for QModelIndex (ptr only), QAbstra...
#include <qabstractitemview.h>    // for QAbstractItemView
#include <qitemselectionmodel.h>  // for QItemSelection (ptr only), QIte...
#include <qmainwindow.h>          // for QMainWindow
#include <qnamespace.h>           // for WindowFlags
#include <qspinbox.h>             // for QDoubleSpinBox
#include <qstring.h>              // for QString
#include <qtmetamacros.h>         // for Q_OBJECT
#include <qvariant.h>             // for QVariant

#include <string>  // for string

#include "justly/Chord.h"             // for Chord
#include "justly/Instrument.h"        // for Instrument
#include "justly/InstrumentEditor.h"  // for InstrumentEditor
#include "justly/Interval.h"          // for Interval
#include "justly/Song.h"              // for Song
#include "justly/StartingField.h"     // for StartingField, starting_instrum...
#include "justly/TreeLevel.h"         // for TreeLevel

class ChordsModel;
class QUndoStack;

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

  double current_time = 0;

  fluid_event_t* event_pointer = new_fluid_event();
  fluid_settings_t* settings_pointer = new_fluid_settings();
  fluid_synth_t* synth_pointer = nullptr;
  fluid_sequencer_t* sequencer_pointer = new_fluid_sequencer2(0);
  fluid_audio_driver_t* audio_driver_pointer = nullptr;
  fluid_seq_id_t sequencer_id = -1;
  int soundfont_id = -1;

  inline void initialize_player() {
    current_key = song.starting_key;
    current_volume = song.starting_volume / PERCENT;
    current_tempo = song.starting_tempo;
    current_time = fluid_sequencer_get_tick(sequencer_pointer);
    current_instrument_pointer = song.starting_instrument_pointer;
  }

  inline void modulate(const Chord* chord_pointer) {
    current_key = current_key * chord_pointer->interval.ratio();
    current_volume = current_volume * chord_pointer->volume_percent / PERCENT;
    current_tempo = current_tempo * chord_pointer->tempo_percent / PERCENT;
    const auto& chord_instrument_pointer = chord_pointer->instrument_pointer;
    if (!chord_instrument_pointer->instrument_name.empty()) {
      current_instrument_pointer = chord_instrument_pointer;
    }
  }

  void play_notes(const Chord* chord_pointer, int first_note_index = 0,
                  int number_of_notes = -1) const;

  [[nodiscard]] inline auto beat_time() const -> double {
    return SECONDS_PER_MINUTE / current_tempo;
  }

  void export_recording();
  void open();
  void save_as();

  void set_starting_instrument(int);
  void set_starting_value(StartingField, const QVariant&);

  void fix_selection(const QItemSelection&, const QItemSelection&);

  void paste(int, const QModelIndex&);

  void update_actions();

  inline void start_real_time() {
#ifdef __linux__
    fluid_settings_setstr(settings_pointer, "audio.driver", "pulseaudio");
#endif
    audio_driver_pointer =
        new_fluid_audio_driver(settings_pointer, synth_pointer);
  }
  void play_chords(int first_chord_index = 0, int number_of_chords = -1);

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

  [[nodiscard]] inline auto get_current_file() const -> const QString& {
    return current_file;
  }

  inline void set_starting_control(StartingField value_type,
                                   const QVariant& new_value,
                                   bool no_signals = false) {
    if (value_type == starting_key_id) {
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
    } else if (value_type == starting_volume_id) {
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
    } else if (value_type == starting_tempo_id) {
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
    } else if (value_type == starting_instrument_id) {
      const auto* new_instrument_pointer = new_value.value<const Instrument*>();
      if (starting_instrument_editor_pointer->value() !=
          new_instrument_pointer) {
        if (no_signals) {
          starting_instrument_editor_pointer->blockSignals(true);
        }
        starting_instrument_editor_pointer->setValue(new_instrument_pointer);
        if (no_signals) {
          starting_instrument_editor_pointer->blockSignals(false);
        }
      }
      song.starting_instrument_pointer = new_instrument_pointer;
    }
  }

  [[nodiscard]] inline auto get_selected_rows() const -> QModelIndexList {
    return chords_view_pointer->selectionModel()->selectedRows();
  }

  [[nodiscard]] inline auto starting_value(StartingField value_type) const
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
  void export_to(const std::string& output_file);

  inline void stop_playing() {
    fluid_sequencer_remove_events(sequencer_pointer, -1, -1, -1);
    for (auto channel_number = 0; channel_number < NUMBER_OF_MIDI_CHANNELS;
         channel_number = channel_number + 1) {
      fluid_event_all_sounds_off(event_pointer, channel_number);
      fluid_sequencer_send_now(sequencer_pointer, event_pointer);
    }
  }
};
