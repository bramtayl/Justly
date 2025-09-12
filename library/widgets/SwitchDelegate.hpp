#pragma once

#include <QtWidgets/QComboBox>
#include <QtWidgets/QStyledItemDelegate>

#include "cell_editors/ProgramEditor.hpp"
#include "cell_editors/VoiceEditor.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "other/Song.hpp"
#include "rows/RowType.hpp"

template <VoiceInterface SubVoice>
[[nodiscard]] static auto get_voice_names(const QList<SubVoice> &voices) {
  QList<QString> names({""});
  std::transform(voices.cbegin(), voices.cend(), std::back_inserter(names),
                 [](const SubVoice &voice) { return voice.name; });
  return names;
}

static auto get_pitched_voice_editor(QWidget *parent_pointer,
                                     const Song &song) {
  return new VoiceEditor( // NOLINT(cppcoreguidelines-owning-memory)
      parent_pointer, get_voice_names(song.pitched_voices));
}

static auto get_unpitched_voice_editor(QWidget *parent_pointer,
                                       const Song &song) {
  return new VoiceEditor( // NOLINT(cppcoreguidelines-owning-memory)
      parent_pointer, get_voice_names(song.unpitched_voices));
}

struct SwitchDelegate : public QStyledItemDelegate {
  Song &song;
  RowType current_row_type = chord_type;
  QStringListModel string_model;

  explicit SwitchDelegate(Song &song_input, QWidget *parent)
      : QStyledItemDelegate(parent), song(song_input) {}

  auto createEditor(QWidget *parent_pointer, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const -> QWidget * override {
    const auto column = index.column();
    if (current_row_type == chord_type) {
      if (column == pitched_note_voice_column) {
        return get_pitched_voice_editor(parent_pointer, song);
      }
      if (column == unpitched_note_voice_column) {
        return get_unpitched_voice_editor(parent_pointer, song);
      }
    }
    if (current_row_type == pitched_note_type &&
        column == pitched_note_voice_column) {
      return get_pitched_voice_editor(parent_pointer, song);
    }
    if (current_row_type == unpitched_note_type &&
        column == unpitched_note_voice_column) {
      return get_unpitched_voice_editor(parent_pointer, song);
    }
    if (current_row_type == pitched_voice_type &&
        column == pitched_voice_instrument_column) {
      return new ProgramEditor( // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, true);
    }
    if (current_row_type == unpitched_voice_type &&
        column == unpitched_voice_percussion_set_column) {
      return new ProgramEditor( // NOLINT(cppcoreguidelines-owning-memory)
          parent_pointer, false);
    }
    return QStyledItemDelegate::createEditor(parent_pointer, option, index);
  };
};