#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <concepts>
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

// a note whose voice_number was (or will be) shifted by a fixed delta, so the
// pre-shift voice_number can always be recovered as current - delta; used
// where that delta is known at both redo and undo, so there's no need to
// separately store the old voice number
template <VoiceInterface SubVoice> struct RenumberedVoiceNote {
  int chord_number;
  int note_number;
};

// a note whose voice_number was overwritten with a value that doesn't encode
// the original (e.g. reassigned to the first remaining voice), so the old
// voice number must be stored to be restorable on undo
template <VoiceInterface SubVoice> struct AffectedVoiceNote {
  int chord_number;
  int note_number;
  int old_voice_number;
};

// finds every note referencing a voice at or after first_affected_voice_number,
// so InsertVoiceRow can shift its voice_number by a known delta on undo/redo
template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto
find_affected_notes(QList<Chord> &chords,
                    const int first_affected_voice_number)
    -> QList<RenumberedVoiceNote<SubVoice>> {
  QList<RenumberedVoiceNote<SubVoice>> affected_notes;
  for (auto chord_number = 0; chord_number < chords.size();
      chord_number = chord_number + 1) {
    auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
        note_number = note_number + 1) {
      if (notes.at(note_number).voice_number >= first_affected_voice_number) {
        affected_notes.push_back({chord_number, note_number});
      }
    }
  }
  return affected_notes;
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
static void offset_voice_numbers(
    QList<Chord> &chords,
    const QList<RenumberedVoiceNote<SubVoice>> &affected_notes,
    const int delta) {
  for (const auto &affected_note : affected_notes) {
    get_voice_notes<SubVoice, SubNote>(
        chords[affected_note.chord_number])[affected_note.note_number]
        .voice_number += delta;
  }
}

// copied notes on the OS clipboard bake in a voice_number that is a plain
// positional index (PitchedNote.hpp/UnpitchedNote.hpp column_to_xml), so
// inserting/removing a voice row must renumber it the same way it renumbers
// notes still in song.chords, or a later paste would silently land on the
// wrong voice. Called on both redo and undo (with is_insertion inverted) so
// that whatever happens to be on the clipboard at the time - including a
// fresh copy made after the original action - gets renumbered correctly,
// rather than blowing away a newer copy by restoring stale bytes. When
// is_insertion is false, voice numbers pointing into the removed range
// collapse to 0, mirroring how notes still in song.chords get reassigned to
// the first remaining voice.
template <NoteInterface SubNote>
static void renumber_clipboard_voice_numbers(const int first_row_number,
                                             const int number_of_rows,
                                             const bool is_insertion) {
  const auto last_removed_row = first_row_number + number_of_rows - 1;

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
            const auto voice_number = xml_to_int(note_field_node);
            auto new_voice_number = voice_number;
            if (is_insertion) {
              if (voice_number >= first_row_number) {
                new_voice_number = voice_number + number_of_rows;
              }
            } else if (voice_number > last_removed_row) {
              new_voice_number = voice_number - number_of_rows;
            } else if (voice_number >= first_row_number) {
              new_voice_number = 0;
            }
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
