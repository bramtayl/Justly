#pragma once

#include "models/ChordsModel.hpp"
#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/PercussionInstrumentEditor.hpp"
#include "cell_editors/ProgramEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "tables/MyTable.hpp"

struct ChordsTable : public MyTable {
  ChordsModel model;
  ChordsTable(QUndoStack &undo_stack, Song &song)
      : model(ChordsModel(undo_stack, song)) {
    const auto &interval_size = get_minimum_size<IntervalEditor>();
    const auto &rational_size = get_minimum_size<RationalEditor>();
    const auto &instrument_size = get_minimum_size<ProgramEditor>();
    const auto &percussion_instrument_size =
        get_minimum_size<PercussionInstrumentEditor>();

    const auto rational_width = rational_size.width();

    set_model(*this, model);

    setColumnWidth(chord_instrument_column, instrument_size.width());
    setColumnWidth(chord_percussion_instrument_column,
                   percussion_instrument_size.width());
    setColumnWidth(chord_interval_column, interval_size.width());
    setColumnWidth(chord_beats_column, rational_width);
    setColumnWidth(chord_velocity_ratio_column, rational_width);
    setColumnWidth(chord_tempo_ratio_column, rational_width);
    resizeColumnToContents(chord_pitched_notes_column);
    resizeColumnToContents(chord_unpitched_notes_column);
    setColumnWidth(chord_words_column, WORDS_WIDTH);

    get_reference(verticalHeader())
        .setDefaultSectionSize(std::max(
            {instrument_size.height(), percussion_instrument_size.height(),
             rational_size.height(), interval_size.height()}));
  }
};