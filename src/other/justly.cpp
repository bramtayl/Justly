#include "justly/justly.hpp"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QAction>
#include <QMetaObject>
#include <QMetaProperty> // IWYU pragma: keep
#include <QMetaType>
#include <QSpinBox>
#include <QString>
#include <QStyleOption>
#include <QTableView>
#include <QTextStream>
#include <QUndoStack>
#include <QWidget>
#include <QtGlobal>
#include <fluidsynth.h>

#include "chord/ChordsModel.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "justly/ChordColumn.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "pitched_note/PitchedNotesModel.hpp"
#include "rational/Rational.hpp"
#include "song/SongEditor.hpp"
#include "unpitched_note/UnpitchedNotesModel.hpp"

void register_converters() {
  QMetaType::registerConverter<Rational, QString>(
      [](const Rational &rational) -> QString {
        auto numerator = rational.numerator;
        auto denominator = rational.denominator;

        QString result;
        QTextStream stream(&result);
        if (numerator != 1) {
          stream << numerator;
        }
        if (denominator != 1) {
          stream << "/" << denominator;
        }
        return result;
      });
  QMetaType::registerConverter<Interval, QString>(
      [](const Interval &interval) -> QString {
        auto numerator = interval.numerator;
        auto denominator = interval.denominator;
        auto octave = interval.octave;

        QString result;
        QTextStream stream(&result);
        if (numerator != 1) {
          stream << numerator;
        }
        if (denominator != 1) {
          stream << "/" << denominator;
        }
        if (octave != 0) {
          stream << "o" << octave;
        }
        return result;
      });
  QMetaType::registerConverter<const Instrument *, QString>(
      [](const Instrument *instrument_pointer) -> QString {
        if (instrument_pointer == nullptr) {
          return "";
        };
        return instrument_pointer->name;
      });
  QMetaType::registerConverter<const PercussionInstrument *, QString>(
      [](const PercussionInstrument *percussion_pointer) -> QString {
        if (percussion_pointer == nullptr) {
          return "";
        };
        return percussion_pointer->name;
      });
  QMetaType::registerConverter<const PercussionSet *, QString>(
      [](const PercussionSet *percussion_set_pointer) -> QString {
        if (percussion_set_pointer == nullptr) {
          return "";
        };
        return percussion_set_pointer->name;
      });
}

auto make_song_editor() -> SongEditor * {
  return new SongEditor; // NOLINT(cppcoreguidelines-owning-memory)
}

void show_song_editor(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->show();
}

void delete_song_editor(SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  delete song_editor_pointer; // NOLINT(cppcoreguidelines-owning-memory)
}

auto get_table_view_pointer(const SongEditor *song_editor_pointer)
    -> QAbstractItemView * {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->table_view_pointer;
}

auto get_chords_model_pointer(const SongEditor *song_editor_pointer)
    -> QAbstractItemModel * {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer;
};

auto get_pitched_notes_model_pointer(const SongEditor *song_editor_pointer)
    -> QAbstractItemModel * {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->pitched_notes_model_pointer;
};

auto get_unpitched_notes_model_pointer(const SongEditor *song_editor_pointer)
    -> QAbstractItemModel * {
  return song_editor_pointer->unpitched_notes_model_pointer;
};

void trigger_edit_pitched_notes(SongEditor *song_editor_pointer,
                                int chord_number) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->table_view_pointer->doubleClicked(
      song_editor_pointer->chords_model_pointer->index(
          chord_number, chord_pitched_notes_column));
};

void trigger_edit_unpitched_notes(SongEditor *song_editor_pointer,
                                  int chord_number) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->table_view_pointer->doubleClicked(
      song_editor_pointer->chords_model_pointer->index(
          chord_number, chord_unpitched_notes_column));
};

void trigger_back_to_chords(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->back_to_chords_action_pointer->trigger();
};

auto get_gain(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return fluid_synth_get_gain(song_editor_pointer->synth_pointer);
};

auto get_starting_key(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_key;
};

auto get_starting_velocity(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_velocity;
};

auto get_starting_tempo(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model_pointer->starting_tempo;
};

auto get_current_file(const SongEditor *song_editor_pointer) -> QString {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->current_file;
};

void set_gain(const SongEditor *song_editor_pointer, double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->gain_editor_pointer->setValue(new_value);
};

void set_starting_key(const SongEditor *song_editor_pointer, double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_key_editor_pointer->setValue(new_value);
}

void set_starting_velocity(const SongEditor *song_editor_pointer,
                           double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_velocity_editor_pointer->setValue(new_value);
}

void set_starting_tempo(const SongEditor *song_editor_pointer,
                        double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_tempo_editor_pointer->setValue(new_value);
}

auto create_editor(const QAbstractItemView *table_view_pointer,
                   QModelIndex index) -> QWidget * {
  Q_ASSERT(table_view_pointer != nullptr);

  auto *delegate_pointer = table_view_pointer->itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  auto *viewport_pointer = table_view_pointer->viewport();
  Q_ASSERT(viewport_pointer != nullptr);

  auto *cell_editor_pointer = delegate_pointer->createEditor(
      viewport_pointer, QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor_pointer != nullptr);

  delegate_pointer->setEditorData(cell_editor_pointer, index);

  return cell_editor_pointer;
}

void set_editor(const QAbstractItemView *table_view_pointer,
                QWidget *cell_editor_pointer, QModelIndex index,
                const QVariant &new_value) {
  Q_ASSERT(table_view_pointer != nullptr);

  Q_ASSERT(cell_editor_pointer != nullptr);
  const auto *cell_editor_meta_object = cell_editor_pointer->metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor_pointer->setProperty(
      cell_editor_meta_object->userProperty().name(), new_value);

  auto *delegate_pointer = table_view_pointer->itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  delegate_pointer->setModelData(cell_editor_pointer,
                                 table_view_pointer->model(), index);
}

void undo(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->undo_stack_pointer->undo();
};

void trigger_insert_after(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->insert_after_action_pointer->trigger();
};
void trigger_insert_into(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->insert_into_action_pointer->trigger();
};
void trigger_delete(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->delete_action_pointer->trigger();
};
void trigger_remove_rows(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->remove_rows_action_pointer->trigger();
};
void trigger_cut(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->cut_action_pointer->trigger();
};
void trigger_copy(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->copy_action_pointer->trigger();
};
void trigger_paste_over(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_over_action_pointer->trigger();
};
void trigger_paste_into(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_into_action_pointer->trigger();
};
void trigger_paste_after(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_after_action_pointer->trigger();
};
void trigger_save(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->save_action_pointer->trigger();
};

void trigger_play(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->play_action_pointer->trigger();
};

void trigger_stop_playing(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->stop_playing_action_pointer->trigger();
};

void open_file(SongEditor *song_editor_pointer, const QString &filename) {
  song_editor_pointer->open_file(filename);
};
void save_as_file(SongEditor *song_editor_pointer, const QString &filename) {
  song_editor_pointer->save_as_file(filename);
};
void export_to_file(SongEditor *song_editor_pointer,
                    const QString &output_file) {
  song_editor_pointer->export_to_file(output_file);
};
