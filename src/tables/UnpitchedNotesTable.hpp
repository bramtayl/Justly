#pragma once

#include "tables/MyTable.hpp"
#include "cell_editors/PercussionInstrumentEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "models/UnpitchedNotesModel.hpp"

struct UnpitchedNotesTable : public MyTable {
  UnpitchedNotesModel model;
  explicit UnpitchedNotesTable(QUndoStack &undo_stack)
      : model(UnpitchedNotesModel(undo_stack)) {
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &percussion_instrument_size =
        get_minimum_size<PercussionInstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(unpitched_note_percussion_instrument_column,
                   percussion_instrument_size.width());
    setColumnWidth(unpitched_note_beats_column, rational_width);
    setColumnWidth(unpitched_note_velocity_ratio_column, rational_width);
    setColumnWidth(unpitched_note_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(std::max(
            {rational_size.height(), percussion_instrument_size.height()}));
  }
};
