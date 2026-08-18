#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtWidgets/QMessageBox>
#include <concepts>
#include <libxml/parser.h>
#include <string>

#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"
#include "xml/XMLDocument.hpp"

class QWidget;
struct PitchedVoice;

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto get_voice_notes(Chord &chord) -> QList<SubNote> & {
  if constexpr (std::same_as<SubVoice, PitchedVoice>) {
    return chord.pitched_notes;
  } else {
    return chord.unpitched_notes;
  }
}

// walks every note across every chord once, calling function(chord_number,
// note_number, voice_number) for each; shared by InsertVoiceRow and
// RemoveVoiceRows, which otherwise each hand-roll the same nested loop to
// sort notes by how their voice_number relates to the affected row range
template <VoiceInterface SubVoice, NoteInterface SubNote, typename Function>
static void for_each_voice_note(QList<Chord> &chords, Function function) {
  for (auto chord_number = 0; chord_number < chords.size();
      chord_number = chord_number + 1) {
    auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
        note_number = note_number + 1) {
      function(chord_number, note_number, notes.at(note_number).voice_number);
    }
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

// looks for a direct voice_number child of note_node and renumbers it (or,
// for a removal that swallows it, zeroes it) the same way
// InsertVoiceRow/RemoveVoiceRows renumber notes still in song.chords; used
// both for a flat copied note row and for a single pitched_note/unpitched_note
// nested inside a copied chord. reassigned_count is bumped whenever a
// voice_number collapses to 0, so the caller can warn about it the same way
// RemoveVoiceRows warns about a live note being reassigned
static void
find_and_process_voice_number(xmlNode &note_node, const int first_row_number,
                              const int last_removed_row,
                              const int number_of_rows, const bool is_insertion,
                              bool &changed, int &reassigned_count) {
  auto *note_field_pointer = xmlFirstElementChild(&note_node);
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
        reassigned_count = reassigned_count + 1;
      }
      xmlNodeSetContent(
          &note_field_node,
          c_string_to_xml_string(std::to_string(new_voice_number).c_str()));
      changed = true;
    }
    note_field_pointer = xmlNextElementSibling(note_field_pointer);
  }
}

// shared by RemoveVoiceRows and renumber_clipboard_voice_numbers so a live
// note reassignment and a stale clipboard reassignment are reported
// identically, aside from is_clipboard calling out that the latter only
// affects notes sitting on the OS clipboard
template <NoteInterface SubNote>
static void warn_reassigned_voices(QWidget &parent,
                                   const int reassigned_count,
                                   const QString &first_voice_name,
                                   const bool is_clipboard = false) {
  QString message;
  QTextStream stream(&message);
  stream << QObject::tr("Reassigning ") << reassigned_count
         << (is_clipboard ? QObject::tr(" clipboard ") : QObject::tr(" "))
         << QObject::tr(SubNote::get_pitched()) << QObject::tr(" note")
         << (reassigned_count == 1 ? QObject::tr(" voice") : QObject::tr(" voices"))
         << QObject::tr(" to the first voice \"") << first_voice_name
         << QObject::tr("\"");
  QMessageBox::warning(&parent, QObject::tr("Voice removed"), message);
}

// copied notes on the OS clipboard bake in a voice_number that is a plain
// positional index (PitchedNote.hpp/UnpitchedNote.hpp column_to_xml), so
// inserting/removing a voice row must renumber it the same way it renumbers
// notes still in song.chords, or a later paste would silently land on the
// wrong voice. A copied chord bakes the same voice_number into any nested
// pitched_note/unpitched_note under its pitched_notes/unpitched_notes
// wrapper, so those are checked too. Called on both redo and undo (with
// is_insertion inverted) so that whatever happens to be on the clipboard at
// the time - including a fresh copy made after the original action - gets
// renumbered correctly, rather than blowing away a newer copy by restoring
// stale bytes. When is_insertion is false, voice numbers pointing into the
// removed range collapse to 0, mirroring how notes still in song.chords get
// reassigned to the first remaining voice; when parent is non-null, that
// collapse is reported with the same warning RemoveVoiceRows shows for a
// live note being reassigned, so a stale clipboard reassignment isn't silent
template <NoteInterface SubNote>
static void
renumber_clipboard_voice_numbers(const int first_row_number,
                                 const int number_of_rows,
                                 const bool is_insertion,
                                 QWidget *parent = nullptr,
                                 const QString &first_voice_name = QString()) {
  const auto last_removed_row = first_row_number + number_of_rows - 1;

  const auto *notes_mime_type = SubNote::get_cells_mime();
  const auto *chords_mime_type = Chord::get_cells_mime();
  const auto notes_container_name =
      std::string(SubNote::get_pitched()) + "_notes";

  // the offscreen QPA platform (used in tests) returns nullptr here until
  // something has actually been copied; a real windowing platform always
  // returns a QMimeData, possibly with zero formats
  const auto *mime_data_pointer = get_clipboard().mimeData();
  if (mime_data_pointer == nullptr) {
    return;
  }
  const auto has_notes_mime = mime_data_pointer->hasFormat(notes_mime_type);
  const auto has_chords_mime =
      !has_notes_mime && mime_data_pointer->hasFormat(chords_mime_type);
  if (!has_notes_mime && !has_chords_mime) {
    return;
  }
  const auto *mime_type = has_notes_mime ? notes_mime_type : chords_mime_type;

  auto document = read_xml_document(mime_data_pointer->data(mime_type));
  if (document.internal_pointer == nullptr) {
    return;
  }
  auto changed = false;
  auto reassigned_count = 0;
  auto *field_pointer = xmlFirstElementChild(&get_root(document));
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (has_notes_mime && name == "left_column") {
      // voice_number is always column 0, so if the copied range starts after
      // it, a paste can never touch voice_number and there is nothing to fix
      if (xml_to_int(field_node) > 0) {
        return;
      }
    } else if (name == "rows") {
      auto *row_pointer = xmlFirstElementChild(&field_node);
      while (row_pointer != nullptr) {
        if (has_notes_mime) {
          find_and_process_voice_number(get_reference(row_pointer),
                                        first_row_number, last_removed_row,
                                        number_of_rows, is_insertion, changed,
                                        reassigned_count);
        } else {
          auto *chord_field_pointer = xmlFirstElementChild(row_pointer);
          while (chord_field_pointer != nullptr) {
            if (get_xml_name(get_reference(chord_field_pointer)) ==
                notes_container_name) {
              auto *note_pointer = xmlFirstElementChild(chord_field_pointer);
              while (note_pointer != nullptr) {
                find_and_process_voice_number(
                    get_reference(note_pointer), first_row_number,
                    last_removed_row, number_of_rows, is_insertion, changed,
                    reassigned_count);
                note_pointer = xmlNextElementSibling(note_pointer);
              }
            }
            chord_field_pointer = xmlNextElementSibling(chord_field_pointer);
          }
        }
        row_pointer = xmlNextElementSibling(row_pointer);
      }
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }

  if (!changed) {
    return;
  }

  if (reassigned_count > 0 && parent != nullptr) {
    warn_reassigned_voices<SubNote>(*parent, reassigned_count,
                                    first_voice_name, /*is_clipboard=*/true);
  }

  auto &new_mime_data = *(new QMimeData); // NOLINT(cppcoreguidelines-owning-memory)
  new_mime_data.setData(mime_type, document_to_byte_array(document));
  get_clipboard().setMimeData(&new_mime_data);
}
