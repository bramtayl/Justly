#pragma once

#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtWidgets/QMenu>

#include "actions/InsertRemoveRows.hpp"
#include "actions/OrphanedVoiceNameLocation.hpp"
#include "actions/SetCells.hpp"
#include "other/Cells.hpp"
#include "widgets/WindowBody.hpp"
#include "xml/XMLChildren.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidator.hpp"

[[nodiscard]] auto get_mime_description(const QString& mime_type) -> QString;

// moves notes onto the first voice if their voice_name is empty (the voice
// column wasn't copied) or matches no voice, e.g. copied from another song,
// or before their voice was renamed or removed; returns how many were the
// latter, so the caller can warn about them
template <NoteInterface SubNote, VoiceInterface SubVoice>
[[nodiscard]] static auto reassign_unknown_voices(QList<SubNote>& notes,
                                                  const QList<SubVoice>& voices)
    -> int {
  auto reassigned_count = 0;
  for (auto& note : notes) {
    auto& voice_name = note.voice_name;
    if (voice_name.isEmpty()) {
      voice_name = voices.at(0).name;
    } else if (!has_voice(voices, voice_name)) {
      voice_name = voices.at(0).name;
      reassigned_count = reassigned_count + 1;
    }
  }
  return reassigned_count;
}

template <NoteInterface SubNote, VoiceInterface SubVoice>
static void maybe_warn_reassigned_voices(QWidget& parent,
                                         const int reassigned_count,
                                         const QList<SubVoice>& voices) {
  if (reassigned_count > 0) {
    warn_reassigned_voices<SubNote>(parent, reassigned_count, voices.at(0).name,
                                    /*is_clipboard=*/true);
  }
}

// pasted notes, including any nested in pasted chords, whose voice no longer
// exists land on the first voice, with a warning
template <RowInterface SubRow>
static void reassign_unknown_pasted_voices(QWidget& parent, const Song& song,
                                           QList<SubRow>& rows) {
  const auto& pitched_voices = song.pitched_voices;
  const auto& unpitched_voices = song.unpitched_voices;
  if constexpr (std::same_as<SubRow, PitchedNote>) {
    maybe_warn_reassigned_voices<PitchedNote>(
        parent, reassign_unknown_voices(rows, pitched_voices), pitched_voices);
  } else if constexpr (std::same_as<SubRow, UnpitchedNote>) {
    maybe_warn_reassigned_voices<UnpitchedNote>(
        parent, reassign_unknown_voices(rows, unpitched_voices),
        unpitched_voices);
  } else if constexpr (std::same_as<SubRow, Chord>) {
    auto pitched_count = 0;
    auto unpitched_count = 0;
    for (auto& chord : rows) {
      pitched_count = pitched_count + reassign_unknown_voices(
                                          chord.pitched_notes, pitched_voices);
      unpitched_count =
          unpitched_count +
          reassign_unknown_voices(chord.unpitched_notes, unpitched_voices);
    }
    maybe_warn_reassigned_voices<PitchedNote>(parent, pitched_count,
                                              pitched_voices);
    maybe_warn_reassigned_voices<UnpitchedNote>(parent, unpitched_count,
                                                unpitched_voices);
  }
}

template <RowInterface SubRow>
[[nodiscard]] static auto parse_clipboard(
    QWidget& parent, const Song& song,
    const int max_rows = std::numeric_limits<int>::max())
    -> std::optional<Cells<SubRow>> {
  const auto& mime_data = get_reference(get_clipboard().mimeData());
  const auto* mime_type = SubRow::get_cells_mime();
  if (!mime_data.hasFormat(mime_type)) {
    const auto formats = mime_data.formats();
    if (formats.empty()) {
      QMessageBox::warning(&parent, QObject::tr("Empty paste error"),
                           QObject::tr("Nothing to paste!"));
      return {};
    };
    QString message;
    QTextStream stream(&message);
    stream << QObject::tr("Cannot paste ") << get_mime_description(formats[0])
           << QObject::tr(" as ") << get_mime_description(mime_type);
    QMessageBox::warning(&parent, QObject::tr("MIME type error"), message);
    return {};
  }

  auto document = read_xml_document(mime_data.data(mime_type));
  if (document.internal_pointer == nullptr) {
    QMessageBox::warning(&parent, QObject::tr("Paste error"),
                         QObject::tr("Invalid XML"));
    return {};
  }

  static XMLValidator clipboard_validator(SubRow::get_clipboard_schema());

  if (validate_against_schema(clipboard_validator, document) != 0) {
    QMessageBox::warning(&parent, QObject::tr("Validation Error"),
                         QObject::tr("Invalid clipboard"));
    return {};
  }

  QList<SubRow> new_rows;
  auto left_column = 0;
  auto right_column = 0;

  for (auto& field_node : get_xml_children(get_root(document))) {
    const auto name = get_xml_name(field_node);
    if (name == "left_column") {
      left_column = xml_to_int(field_node);
    } else if (name == "right_column") {
      right_column = xml_to_int(field_node);
    } else {
      Q_ASSERT(name == "rows");
      for (auto& xml_row :
           get_xml_children(field_node) | std::views::take(max_rows)) {
        SubRow child_row;
        child_row.from_xml(xml_row);
        new_rows.push_back(std::move(child_row));
      }
    }
  }
  reassign_unknown_pasted_voices(parent, song, new_rows);
  return Cells(left_column, right_column, std::move(new_rows));
}

template <RowInterface SubRow>
[[nodiscard]] static auto make_paste_insert_command(
    QWidget& parent, RowsModel<SubRow>& rows_model, const int row_number)
    -> QUndoCommand* {
  auto maybe_cells = parse_clipboard<SubRow>(parent, rows_model.song);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto& cells = maybe_cells.value();
  return new InsertRemoveRows(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(cells.rows), cells.left_column,
      cells.right_column, false);
}

template <RowInterface SubRow>
[[nodiscard]] static auto make_paste_cells_command(
    QWidget& parent, const int first_row_number, RowsModel<SubRow>& rows_model)
    -> QUndoCommand* {
  auto& rows = rows_model.get_rows();
  auto maybe_cells =
      parse_clipboard<SubRow>(parent, rows_model.song,
                              static_cast<int>(rows.size()) - first_row_number);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto& cells = maybe_cells.value();
  // Justly never copies voice names, but another program could put them on
  // the clipboard, and pasted names could be empty or duplicated
  if constexpr (VoiceInterface<SubRow>) {
    const auto name_column = SubRow::get_name_column();
    if (cells.left_column <= name_column && name_column <= cells.right_column) {
      QMessageBox::warning(&parent, QObject::tr("Paste error"),
                           QObject::tr("Cannot paste voice names!"));
      return nullptr;
    }
  }
  auto& copy_rows = cells.rows;
  return new SetCells(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, static_cast<int>(copy_rows.size()),
      cells.left_column, cells.right_column, std::move(copy_rows));
}

struct PasteMenu : public QMenu {
  QAction paste_over_action;
  QAction paste_into_start_action;
  QAction paste_after_action;

  explicit PasteMenu(WindowBody& window_body);
};
