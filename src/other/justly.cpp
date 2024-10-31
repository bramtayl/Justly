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
#include "named/Named.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "pitched_note/PitchedNotesModel.hpp"
#include "rational/Rational.hpp"
#include "rows/RowsModel.hpp"
#include "song/Player.hpp"
#include "song/Song.hpp"
#include "song/SongEditor.hpp"

void register_converters() {
  QMetaType::registerConverter<Rational, QString>([](const Rational &rational) {
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
  QMetaType::registerConverter<Interval, QString>([](const Interval &interval) {
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
  QMetaType::registerConverter<const Instrument *, QString>(&get_name_or_empty);
  QMetaType::registerConverter<const PercussionInstrument *, QString>(
      &get_name_or_empty);
  QMetaType::registerConverter<const PercussionSet *, QString>(
      &get_name_or_empty);
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

auto get_table_view(const SongEditor *song_editor_pointer)
    -> QAbstractItemView & {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->table_view;
}

auto get_chords_model(const SongEditor *song_editor_pointer)
    -> QAbstractItemModel & {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->chords_model;
};

auto get_pitched_notes_model(const SongEditor *song_editor_pointer)
    -> QAbstractItemModel & {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->pitched_notes_model;
};

auto get_unpitched_notes_model(const SongEditor *song_editor_pointer)
    -> QAbstractItemModel & {
  return song_editor_pointer->unpitched_notes_model;
};

void trigger_edit_pitched_notes(SongEditor *song_editor_pointer,
                                int chord_number) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->table_view.doubleClicked(
      song_editor_pointer->chords_model.index(chord_number,
                                              chord_pitched_notes_column));
};

void trigger_edit_unpitched_notes(SongEditor *song_editor_pointer,
                                  int chord_number) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->table_view.doubleClicked(
      song_editor_pointer->chords_model.index(chord_number,
                                              chord_unpitched_notes_column));
};

void trigger_back_to_chords(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->back_to_chords_action.trigger();
};

auto get_gain(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return fluid_synth_get_gain(song_editor_pointer->player.synth_pointer);
};

auto get_starting_key(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->song.starting_key;
};

auto get_starting_velocity(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->song.starting_velocity;
};

auto get_starting_tempo(const SongEditor *song_editor_pointer) -> double {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->song.starting_tempo;
};

auto get_current_file(const SongEditor *song_editor_pointer) -> QString {
  Q_ASSERT(song_editor_pointer != nullptr);
  return song_editor_pointer->current_file;
};

void set_gain(const SongEditor *song_editor_pointer, double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->gain_editor.setValue(new_value);
};

void set_starting_key(const SongEditor *song_editor_pointer, double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_key_editor.setValue(new_value);
}

void set_starting_velocity(const SongEditor *song_editor_pointer,
                           double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_velocity_editor.setValue(new_value);
}

void set_starting_tempo(const SongEditor *song_editor_pointer,
                        double new_value) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->starting_tempo_editor.setValue(new_value);
}

auto create_editor(const QAbstractItemView &table_view,
                   QModelIndex index) -> QWidget & {
  auto *delegate_pointer = table_view.itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);
  auto &delegate = *(delegate_pointer);

  auto *viewport_pointer = table_view.viewport();
  Q_ASSERT(viewport_pointer != nullptr);

  auto *cell_editor =
      delegate.createEditor(viewport_pointer, QStyleOptionViewItem(), index);
  Q_ASSERT(cell_editor != nullptr);

  delegate.setEditorData(cell_editor, index);

  return *cell_editor;
}

void set_editor(const QAbstractItemView &table_view, QWidget &cell_editor,
                QModelIndex index, const QVariant &new_value) {
  const auto *cell_editor_meta_object = cell_editor.metaObject();

  Q_ASSERT(cell_editor_meta_object != nullptr);
  cell_editor.setProperty(cell_editor_meta_object->userProperty().name(),
                          new_value);

  auto *delegate_pointer = table_view.itemDelegate();
  Q_ASSERT(delegate_pointer != nullptr);

  delegate_pointer->setModelData(&cell_editor, table_view.model(), index);
}

void undo(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->undo_stack.undo();
};

void trigger_insert_after(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->insert_after_action.trigger();
};
void trigger_insert_into(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->insert_into_action.trigger();
};
void trigger_delete(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->delete_action.trigger();
};
void trigger_remove_rows(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->remove_rows_action.trigger();
};
void trigger_cut(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->cut_action.trigger();
};
void trigger_copy(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->copy_action.trigger();
};
void trigger_paste_over(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_over_action.trigger();
};
void trigger_paste_into(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_into_action.trigger();
};
void trigger_paste_after(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->paste_after_action.trigger();
};
void trigger_save(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->save_action.trigger();
};

void trigger_play(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->play_action.trigger();
};

void trigger_stop_playing(const SongEditor *song_editor_pointer) {
  Q_ASSERT(song_editor_pointer != nullptr);
  song_editor_pointer->stop_playing_action.trigger();
};

void open_file(SongEditor *song_editor_pointer, const QString &filename) {
  Q_ASSERT(song_editor_pointer != nullptr);
  open_file(*song_editor_pointer, filename);
};
void save_as_file(SongEditor *song_editor_pointer, const QString &filename) {
  Q_ASSERT(song_editor_pointer != nullptr);
  save_as_file(*song_editor_pointer, filename);
};
void export_to_file(SongEditor *song_editor_pointer,
                    const QString &output_file) {
  Q_ASSERT(song_editor_pointer != nullptr);
  export_to_file(*song_editor_pointer, output_file);
};
