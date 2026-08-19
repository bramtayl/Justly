#pragma once

#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>

#include "models/UndoRowsModel.hpp"
#include "rows/Chord.hpp"

class QUndoStack;
class QTextStream;
struct Chord;
struct Song;

struct ChordsModel : public UndoRowsModel<Chord> {
  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input);

  void add_to_status(QTextStream &stream,  int row_number,
                     const Chord &chord) const override;
};
