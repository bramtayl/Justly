#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <concepts>
#include <functional>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlstring.h>
#include <optional>
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

// named distinctly from PasteMenu.hpp's get_clipboard() to avoid a duplicate
// static-function definition when both headers land in the same translation
// unit
[[nodiscard]] static inline auto get_voice_clipboard() -> auto & {
  return get_reference(QGuiApplication::clipboard());
}

// copied notes on the OS clipboard bake in a voice_number that is a plain
// positional index (PitchedNote.hpp/UnpitchedNote.hpp column_to_xml), so
// inserting/removing a voice row must renumber it the same way it renumbers
// notes still in song.chords, or a later paste would silently land on the
// wrong voice. Returns the clipboard's original bytes (for undo) if it held
// SubNote cells with the voice column copied, or nullopt if nothing needed
// changing.
template <NoteInterface SubNote>
[[nodiscard]] static auto renumber_clipboard_voice_numbers(
    const std::function<int(int)> &transform_voice_number)
    -> std::optional<QByteArray> {
  const auto *mime_type = SubNote::get_cells_mime();
  // the offscreen QPA platform (used in tests) returns nullptr here until
  // something has actually been copied; a real windowing platform always
  // returns a QMimeData, possibly with zero formats
  const auto *mime_data_pointer = get_voice_clipboard().mimeData();
  if (mime_data_pointer == nullptr ||
      !mime_data_pointer->hasFormat(mime_type)) {
    return {};
  }
  const auto old_bytes = mime_data_pointer->data(mime_type);

  XMLDocument document(xmlReadMemory(old_bytes.constData(), old_bytes.size(),
                                     nullptr, nullptr, 0));
  if (document.internal_pointer == nullptr) {
    return {};
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
        return {};
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
    return {};
  }

  xmlChar *char_buffer = nullptr;
  auto buffer_size = 0;
  xmlDocDumpMemory(document.internal_pointer, &char_buffer, &buffer_size);
  auto &new_mime_data = *(new QMimeData); // NOLINT(cppcoreguidelines-owning-memory)
  new_mime_data.setData(
      mime_type,
      QByteArray(
          reinterpret_cast<const char *>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
              char_buffer),
          buffer_size));
  get_voice_clipboard().setMimeData(&new_mime_data);
  xmlFree(char_buffer);

  return old_bytes;
}

static void
restore_clipboard_bytes(const char *mime_type,
                        const std::optional<QByteArray> &old_bytes) {
  if (!old_bytes.has_value()) {
    return;
  }
  auto &mime_data = *(new QMimeData); // NOLINT(cppcoreguidelines-owning-memory)
  mime_data.setData(mime_type, old_bytes.value());
  get_voice_clipboard().setMimeData(&mime_data);
}
