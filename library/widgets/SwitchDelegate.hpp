#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <iterator>

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/MidiNumberEditor.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "cell_editors/StringPicker.hpp"
#include "cell_editors/VoiceNumberPicker.hpp"
#include "cell_types/Program.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"
#include "rows/Voice.hpp"

[[nodiscard]] auto create_string_picker(QWidget *parent_pointer,
                                        const QList<QString> &names) -> StringPicker &;

template <VoiceInterface SubVoice>
static auto create_voice_number_picker(QWidget *parent_pointer,
                                       const QList<SubVoice> &voices)
    -> auto & {
  QList<QString> voice_names;
  voice_names.reserve(voices.size());
  std::ranges::transform(voices, std::back_inserter(voice_names),
                         &SubVoice::name);
  auto &specific_result = get_reference(
      new VoiceNumberPicker( // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, voice_names));
  specific_result.setFrame(false);
  return specific_result;
}

struct SwitchDelegate : public QStyledItemDelegate {
  Song &song;
  RowType current_row_type = RowType::chord_type;

  explicit SwitchDelegate(Song &song_input, QWidget *parent)
      : QStyledItemDelegate(parent), song(song_input) {}

  auto createEditor(QWidget *parent_pointer, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const -> QWidget * override;
};
