#pragma once

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/ProgramEditor.hpp"
#include "models/PitchedNotesModel.hpp"
#include "tables/MyTable.hpp"

struct PitchedNotesTable : public MyTable {
  PitchedNotesModel pitched_notes_model;
  PitchedNotesTable(QUndoStack &undo_stack, Song &song)
      : pitched_notes_model(PitchedNotesModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<ProgramEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, pitched_notes_model);

    setColumnWidth(pitched_note_instrument_column, instrument_size.width());
    setColumnWidth(pitched_note_interval_column, interval_size.width());
    setColumnWidth(pitched_note_beats_column, rational_width);
    setColumnWidth(pitched_note_velocity_ratio_column, rational_width);
    setColumnWidth(pitched_note_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(
            std::max({rational_size.height(), instrument_size.height(),
                      interval_size.height()}));
  }
};