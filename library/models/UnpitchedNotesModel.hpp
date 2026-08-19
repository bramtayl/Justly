#pragma once

#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QVariant>

#include "cell_types/Rational.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"
#include "rows/UnpitchedNote.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "sound/PlayState.hpp"

class QUndoStack;

struct UnpitchedNotesModel : public UndoRowsModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack &undo_stack, Song &song)
      : UndoRowsModel<UnpitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto get_display_data(const int row_number,
                                      const int column_number) const
      -> QVariant override;

  void add_to_status(QTextStream &stream, const int /*row_number*/,
                     const UnpitchedNote &unpitched_note) const override;
};
