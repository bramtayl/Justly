#pragma once

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QVariant>

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "sound/PlayState.hpp"

class QUndoStack;

struct PitchedNotesModel : public UndoRowsModel<PitchedNote> {
  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song)
      : UndoRowsModel<PitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto get_display_data(const int row_number,
                                      const int column_number) const
      -> QVariant override;

  void add_to_status(QTextStream &stream, const int /*row_number*/,
                     const PitchedNote &pitched_note) const override;
};
