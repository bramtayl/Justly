#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <concepts>
#include <functional>
#include <libxml/parser.h>
#include <string>

#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"
#include "xml/XMLDocument.hpp"

struct PitchedVoice;

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto get_voice_notes(Chord &chord) -> QList<SubNote> & {
  if constexpr (std::same_as<SubVoice, PitchedVoice>) {
    return chord.pitched_notes;
  } else {
    return chord.unpitched_notes;
  }
}

template <VoiceInterface SubVoice> struct AffectedVoiceNote {
  int chord_number;
  int note_number;
  int old_voice_number;
};

// the transform a clipboard voice_number needs when number_of_rows voice
// rows are inserted at first_row_number, for InsertVoiceRow::redo and
// RemoveVoiceRows::undo (reinserting the rows it removed)
[[nodiscard]] static inline auto
make_insert_voice_transform(const int first_row_number,
                            const int number_of_rows)
    -> std::function<int(int)> {
  return [first_row_number, number_of_rows](const int voice_number) -> int {
    return voice_number >= first_row_number ? voice_number + number_of_rows
                                            : voice_number;
  };
}

// the transform a clipboard voice_number needs when number_of_rows voice
// rows starting at first_row_number are removed, for RemoveVoiceRows::redo
// and InsertVoiceRow::undo (removing the row it inserted); voice numbers
// pointing into the removed range collapse to 0, mirroring how notes still
// in song.chords get reassigned to the first remaining voice
[[nodiscard]] static inline auto
make_remove_voice_transform(const int first_row_number,
                            const int number_of_rows)
    -> std::function<int(int)> {
  const auto last_removed_row = first_row_number + number_of_rows - 1;
  return [first_row_number, last_removed_row,
          number_of_rows](const int voice_number) -> int {
    if (voice_number < first_row_number) {
      return voice_number;
    }
    if (voice_number > last_removed_row) {
      return voice_number - number_of_rows;
    }
    return 0;
  };
}

// finds every note referencing a voice at or after first_affected_voice_number,
// so InsertVoiceRow/RemoveVoiceRows can shift or restore voice_number on
// undo/redo
template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto
find_affected_notes(QList<Chord> &chords,
                    const int first_affected_voice_number)
    -> QList<AffectedVoiceNote<SubVoice>> {
  QList<AffectedVoiceNote<SubVoice>> affected_notes;
  for (auto chord_number = 0; chord_number < chords.size();
      chord_number = chord_number + 1) {
    auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
        note_number = note_number + 1) {
      const auto old_voice_number = notes.at(note_number).voice_number;
      if (old_voice_number >= first_affected_voice_number) {
        affected_notes.push_back(
            {chord_number, note_number, old_voice_number});
      }
    }
  }
  return affected_notes;
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
static void restore_affected_notes(
    QList<Chord> &chords,
    const QList<AffectedVoiceNote<SubVoice>> &affected_notes) {
  for (const auto &affected_note : affected_notes) {
    get_voice_notes<SubVoice, SubNote>(
        chords[affected_note.chord_number])[affected_note.note_number]
        .voice_number = affected_note.old_voice_number;
  }
}

// copied notes on the OS clipboard bake in a voice_number that is a plain
// positional index (PitchedNote.hpp/UnpitchedNote.hpp column_to_xml), so
// inserting/removing a voice row must renumber it the same way it renumbers
// notes still in song.chords, or a later paste would silently land on the
// wrong voice. Called on both redo and undo (with an inverting transform) so
// that whatever happens to be on the clipboard at the time - including a
// fresh copy made after the original action - gets renumbered correctly,
// rather than blowing away a newer copy by restoring stale bytes.
template <NoteInterface SubNote>
static void renumber_clipboard_voice_numbers(
    const std::function<int(int)> &transform_voice_number) {
  const auto *mime_type = SubNote::get_cells_mime();
  // the offscreen QPA platform (used in tests) returns nullptr here until
  // something has actually been copied; a real windowing platform always
  // returns a QMimeData, possibly with zero formats
  const auto *mime_data_pointer = get_clipboard().mimeData();
  if (mime_data_pointer == nullptr ||
      !mime_data_pointer->hasFormat(mime_type)) {
    return;
  }
  auto document = read_xml_document(mime_data_pointer->data(mime_type));
  if (document.internal_pointer == nullptr) {
    return;
  }
  auto &root = get_root(document);

  auto changed = false;
  auto *field_pointer = xmlFirstElementChild(&root);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "left_column") {
      // voice_number is always column 0, so if the copied range starts after
      // it, a paste can never touch voice_number and there is nothing to fix
      if (xml_to_int(field_node) > 0) {
        return;
      }
    } else if (name == "rows") {
      auto *row_pointer = xmlFirstElementChild(&field_node);
      while (row_pointer != nullptr) {
        auto *note_field_pointer = xmlFirstElementChild(row_pointer);
        while (note_field_pointer != nullptr) {
          auto &note_field_node = get_reference(note_field_pointer);
          if (get_xml_name(note_field_node) == "voice_number") {
            const auto new_voice_number =
                transform_voice_number(xml_to_int(note_field_node));
            xmlNodeSetContent(
                &note_field_node,
                c_string_to_xml_string(
                    std::to_string(new_voice_number).c_str()));
            changed = true;
          }
          note_field_pointer = xmlNextElementSibling(note_field_pointer);
        }
        row_pointer = xmlNextElementSibling(row_pointer);
      }
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }

  if (!changed) {
    return;
  }

  auto &new_mime_data = *(new QMimeData); // NOLINT(cppcoreguidelines-owning-memory)
  new_mime_data.setData(mime_type, document_to_byte_array(document));
  get_clipboard().setMimeData(&new_mime_data);
}
