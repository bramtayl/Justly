#pragma once

#include <QtCore/QDebug>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>

#include "cell_types/Rational.hpp"
#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"
#include "rows/Chord.hpp"
#include "sound/PlayState.hpp"

class QUndoStack;

struct ChordsModel : public UndoRowsModel<Chord> {
  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input);

  void add_to_status(QTextStream &stream, const int row_number,
                     const Chord &chord) const override;
};
