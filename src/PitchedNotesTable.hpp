#pragma once

#include "IntervalEditor.hpp"
#include "MyTable.hpp"
#include "PitchedNotesModel.hpp"
#include "ProgramEditor.hpp"

struct PitchedNotesTable : public MyTable {
  PitchedNotesModel model;
  PitchedNotesTable(QUndoStack &undo_stack, Song &song)
      : model(PitchedNotesModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<ProgramEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

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