#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QTextStream>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QAction>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QMenu>
#include <utility>

#include "actions/InsertRow.hpp"
#include "actions/InsertVoiceRow.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/PitchedVoicesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "models/UnpitchedVoicesModel.hpp"
#include "other/Song.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedNote.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "rows/Voice.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"

template <RowInterface SubRow> struct RowsModel;
template <VoiceInterface SubVoice> struct VoicesModel;

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto
make_insert_note(RowsModel<SubNote> &notes_model,
                 const QList<Chord> &chords,
                 const int row_number) -> QUndoCommand * {
  SubNote sub_note;
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  return new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
      notes_model, row_number, std::move(sub_note));
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto
make_insert_voice(VoicesModel<SubVoice> &voices_model,
                  const int row_number) -> QUndoCommand * {
  auto& created_voices = voices_model.created_voices;
  created_voices = created_voices + 1;
  SubVoice sub_voice;
  QTextStream stream(&sub_voice.name);
  stream << SubVoice::get_pitched() << " voice " << created_voices;
  return new InsertVoiceRow< // NOLINT(cppcoreguidelines-owning-memory)
      SubVoice, SubNote>(voices_model, row_number, std::move(sub_voice));
}

void add_insert_row(SongWidget &song_widget, const int row_number,
                    const RowType new_row_type);

void add_insert_row_default(SongWidget &song_widget,
                            const int row_number);

struct InsertMenu : public QMenu {
  QAction insert_after_action = QAction(InsertMenu::tr("&After"));
  QAction insert_into_start_action = QAction(InsertMenu::tr("&Into start"));

  explicit InsertMenu(SongWidget &song_widget);
};
