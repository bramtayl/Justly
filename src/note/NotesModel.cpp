#include "note/NotesModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "chord/ChordsModel.hpp"
#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "items_model/SetBeats.hpp"
#include "items_model/SetInstrument.hpp"
#include "items_model/SetInterval.hpp"
#include "items_model/SetTempoRatio.hpp"
#include "items_model/SetVelocityRatio.hpp"
#include "items_model/SetWords.hpp"
#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"
#include "rational/Rational.hpp"

static const auto NUMBER_OF_NOTE_COLUMNS = 6;

// static functions
[[nodiscard]] static auto
get_note_column(const QModelIndex &index) -> NoteColumn {
  return to_note_column(index.column());
}

// header functions

auto to_note_column(int column) -> NoteColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_NOTE_COLUMNS);
  return static_cast<NoteColumn>(column);
}

NotesModel::NotesModel(ChordsModel *parent_chords_model_pointer_input,
                       QObject *parent_pointer)
    : ItemsModel<Note>(nullptr, parent_pointer),
      parent_chords_model_pointer(parent_chords_model_pointer_input) {
  Q_ASSERT(parent_chords_model_pointer != nullptr);
}

auto NotesModel::get_instrument_column() const -> int {
  return note_instrument_column;
};

auto NotesModel::get_interval_column() const -> int {
  return note_interval_column;
};

auto NotesModel::get_beats_column() const -> int { return note_beats_column; };

auto NotesModel::get_tempo_ratio_column() const -> int {
  return note_tempo_ratio_column;
};

auto NotesModel::get_velocity_ratio_column() const -> int {
  return note_velocity_ratio_column;
};

auto NotesModel::get_words_column() const -> int { return note_words_column; };

auto NotesModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_NOTE_COLUMNS;
}

auto NotesModel::get_column_name(int column_number) const -> QString {
  switch (to_note_column(column_number)) {
  case note_instrument_column:
    return NotesModel::tr("Instrument");
  case note_interval_column:
    return NotesModel::tr("Interval");
  case note_beats_column:
    return NotesModel::tr("Beats");
  case note_velocity_ratio_column:
    return NotesModel::tr("Velocity ratio");
  case note_tempo_ratio_column:
    return NotesModel::tr("Tempo ratio");
  case note_words_column:
    return NotesModel::tr("Words");
  }
}

auto NotesModel::data(const QModelIndex &index, int role) const -> QVariant {
  if (role == Qt::StatusTipRole) {
    Q_ASSERT(items_pointer != nullptr);
    return get_key_text(
        *parent_chords_model_pointer, parent_chord_number,
        interval_to_double(items_pointer->at(index.row()).interval));
  }
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return {};
  }
  Q_ASSERT(items_pointer != nullptr);
  const auto &note = items_pointer->at(index.row());
  switch (get_note_column(index)) {
  case note_instrument_column:
    return QVariant::fromValue(note.instrument_pointer);
  case note_interval_column:
    return QVariant::fromValue(note.interval);
  case note_beats_column:
    return QVariant::fromValue(note.beats);
  case note_velocity_ratio_column:
    return QVariant::fromValue(note.velocity_ratio);
  case note_tempo_ratio_column:
    return QVariant::fromValue(note.tempo_ratio);
  case note_words_column:
    return note.words;
  }
}

auto NotesModel::setData(const QModelIndex &index, const QVariant &new_value,
                         int role) -> bool {
  auto *undo_stack_pointer = parent_chords_model_pointer->undo_stack_pointer;
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto item_number = index.row();
  Q_ASSERT(items_pointer != nullptr);
  const auto &note = items_pointer->at(item_number);
  switch (get_note_column(index)) {
  case note_instrument_column:
    Q_ASSERT(new_value.canConvert<const Instrument *>());
    undo_stack_pointer->push(std::make_unique<SetInstrument<Note>>(
                                 this, item_number, note.instrument_pointer,
                                 new_value.value<const Instrument *>())
                                 .release());
    break;
  case note_interval_column:
    Q_ASSERT(new_value.canConvert<Interval>());
    undo_stack_pointer->push(
        std::make_unique<SetInterval<Note>>(this, item_number, note.interval,
                                            new_value.value<Interval>())
            .release());

    break;
  case note_beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetBeats<Note>>(this, item_number, note.beats,
                                         new_value.value<Rational>())
            .release());
    break;
  case note_velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetVelocityRatio<Note>>(
            this, item_number, note.velocity_ratio, new_value.value<Rational>())
            .release());
    break;
  case note_tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetTempoRatio<Note>>(
            this, item_number, note.tempo_ratio, new_value.value<Rational>())
            .release());
    break;
  case note_words_column:
    Q_ASSERT(new_value.canConvert<QString>());
    undo_stack_pointer->push(
        std::make_unique<SetWords<Note>>(this, item_number, note.words,
                                         new_value.value<QString>())
            .release());
    break;
  default:
    Q_ASSERT(false);
  }
  return true;
}

