#pragma once

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QtGlobal>
#include <QtWidgets/QMainWindow>

class QAbstractItemModel;
class QModelIndex;
class QAbstractItemView;
class QWidget;
struct SongWidget;
struct SongMenuBar;

#if defined(JUSTLY_LIBRARY)
#define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#define JUSTLY_EXPORT Q_DECL_IMPORT
#endif

enum JUSTLY_EXPORT ChordColumn {
  chord_instrument_column,
  chord_percussion_set_column,
  chord_percussion_instrument_column,
  chord_interval_column,
  chord_beats_column,
  chord_velocity_ratio_column,
  chord_tempo_ratio_column,
  chord_words_column,
  chord_pitched_notes_column,
  chord_unpitched_notes_column,
  number_of_chord_columns
};

enum JUSTLY_EXPORT PitchedNoteColumn {
  pitched_note_instrument_column,
  pitched_note_interval_column,
  pitched_note_beats_column,
  pitched_note_velocity_ratio_column,
  pitched_note_words_column,
  number_of_pitched_note_columns
};

enum JUSTLY_EXPORT UnpitchedNoteColumn {
  unpitched_note_percussion_set_column,
  unpitched_note_percussion_instrument_column,
  unpitched_note_beats_column,
  unpitched_note_velocity_ratio_column,
  unpitched_note_words_column,
  number_of_unpitched_note_columns
};

struct JUSTLY_EXPORT SongEditor : public QMainWindow {
public:
  SongWidget &song_widget;
  SongMenuBar &song_menu_bar;

  explicit SongEditor();
  void closeEvent(QCloseEvent *close_event_pointer) override;
};

void JUSTLY_EXPORT set_up();

[[nodiscard]] auto JUSTLY_EXPORT get_table_view(const SongWidget &song_widget)
    -> QAbstractItemView &;

[[nodiscard]] auto JUSTLY_EXPORT get_chords_model(SongWidget &song_widget)
    -> QAbstractItemModel &;
[[nodiscard]] auto JUSTLY_EXPORT
get_pitched_notes_model(SongWidget &song_widget) -> QAbstractItemModel &;
[[nodiscard]] auto JUSTLY_EXPORT
get_unpitched_notes_model(SongWidget &song_widget) -> QAbstractItemModel &;

void JUSTLY_EXPORT trigger_edit_pitched_notes(SongWidget &song_widget,
                                              int chord_number);
void JUSTLY_EXPORT trigger_edit_unpitched_notes(SongWidget &song_widget,
                                                int chord_number);
void JUSTLY_EXPORT trigger_back_to_chords(SongMenuBar &song_menu_bar);

[[nodiscard]] auto JUSTLY_EXPORT get_gain(const SongWidget &song_widget)
    -> double;
[[nodiscard]] auto JUSTLY_EXPORT get_starting_key(const SongWidget &song_widget)
    -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_velocity(const SongWidget &song_widget) -> double;
[[nodiscard]] auto JUSTLY_EXPORT
get_starting_tempo(const SongWidget &song_widget) -> double;

[[nodiscard]] auto JUSTLY_EXPORT get_current_file(const SongWidget &song_widget)
    -> QString;
[[nodiscard]] auto JUSTLY_EXPORT
get_current_chord_number(const SongWidget &song_widget) -> int;

void JUSTLY_EXPORT set_gain(const SongWidget &song_widget, double new_value);
void JUSTLY_EXPORT set_starting_key(const SongWidget &song_widget,
                                    double new_value);
void JUSTLY_EXPORT set_starting_velocity(const SongWidget &song_widget,
                                         double new_value);
void JUSTLY_EXPORT set_starting_tempo(const SongWidget &song_widget,
                                      double new_value);

[[nodiscard]] auto JUSTLY_EXPORT create_editor(
    const QAbstractItemView &table_view, QModelIndex index) -> QWidget &;
void JUSTLY_EXPORT set_editor(const QAbstractItemView &table_view,
                              QWidget &cell_editor_pointer, QModelIndex index,
                              const QVariant &new_value);

void JUSTLY_EXPORT undo(SongWidget &song_widget);

void JUSTLY_EXPORT trigger_previous_chord(SongWidget &song_widget);
void JUSTLY_EXPORT trigger_next_chord(SongWidget &song_widget);

void JUSTLY_EXPORT trigger_insert_after(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_insert_into(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_delete_cells(SongMenuBar &song_menu_ba);
void JUSTLY_EXPORT trigger_remove_rows(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_cut(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_copy(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_paste_over(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_paste_after(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_paste_into(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_save(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT trigger_play(SongMenuBar &song_menu_bar);
void JUSTLY_EXPORT trigger_stop_playing(SongMenuBar &song_menu_bar);

void JUSTLY_EXPORT open_file(SongWidget &song_widget, const QString &filename);
void JUSTLY_EXPORT save_as_file(SongWidget &song_widget,
                                const QString &filename);
void JUSTLY_EXPORT export_to_file(SongWidget &song_widget,
                                  const QString &output_file);