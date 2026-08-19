#pragma once

#include <libxml/parser.h>

#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QtAssert>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QKeySequence>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

#include "actions/InsertRemoveRows.hpp"
#include "actions/SetCells.hpp"
#include "other/Cells.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/Row.hpp"
#include "widgets/SongWidget.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/XMLValidator.hpp"

class QWidget;
template <RowInterface SubRow>
struct RowsModel;
class QUndoCommand;

[[nodiscard]] auto get_mime_description(const QString& mime_type) -> QString;

template <RowInterface SubRow>
[[nodiscard]] static auto parse_clipboard(
    QWidget& parent, const int max_rows = std::numeric_limits<int>::max())
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

  auto* field_pointer = xmlFirstElementChild(&get_root(document));
  while (field_pointer != nullptr) {
    auto& field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "left_column") {
      left_column = xml_to_int(field_node);
    } else if (name == "right_column") {
      right_column = xml_to_int(field_node);
    } else if (name == "rows") {
      auto counter = 1;
      auto* xml_row_pointer = xmlFirstElementChild(&field_node);
      while (xml_row_pointer != nullptr && counter <= max_rows) {
        SubRow child_row;
        child_row.from_xml(get_reference(xml_row_pointer));
        new_rows.push_back(std::move(child_row));
        xml_row_pointer = xmlNextElementSibling(xml_row_pointer);
        counter++;
      }
    } else {
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
  return Cells(left_column, right_column, std::move(new_rows));
}

template <RowInterface SubRow>
[[nodiscard]] static auto make_paste_insert_command(
    QWidget& parent, RowsModel<SubRow>& rows_model, const int row_number)
    -> QUndoCommand* {
  const auto maybe_cells = parse_clipboard<SubRow>(parent);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto& cells = maybe_cells.value();
  if constexpr (NoteInterface<SubRow>) {
    const auto& song = rows_model.song;
    const auto number_of_voices =
        SubRow::is_pitched() ? static_cast<int>(song.pitched_voices.size())
                             : static_cast<int>(song.unpitched_voices.size());
    if (!check_note_voices(parent, cells.rows, number_of_voices,
                           rows_model.parent_chord_number)) {
      return nullptr;
    }
  }
  return new InsertRemoveRows(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(cells.rows), cells.left_column,
      cells.right_column, false);
}

template <RowInterface SubRow>
[[nodiscard]] static auto make_paste_cells_command(
    QWidget& parent, const int first_row_number, RowsModel<SubRow>& rows_model)
    -> QUndoCommand* {
  auto& rows = rows_model.get_rows();
  auto maybe_cells = parse_clipboard<SubRow>(
      parent, static_cast<int>(rows.size()) - first_row_number);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto& cells = maybe_cells.value();
  auto& copy_rows = cells.rows;
  if constexpr (NoteInterface<SubRow>) {
    const auto& song = rows_model.song;
    const auto number_of_voices =
        SubRow::is_pitched() ? static_cast<int>(song.pitched_voices.size())
                             : static_cast<int>(song.unpitched_voices.size());
    if (!check_note_voices(parent, copy_rows, number_of_voices,
                           rows_model.parent_chord_number)) {
      return nullptr;
    }
  }
  return new SetCells(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number, static_cast<int>(copy_rows.size()),
      cells.left_column, cells.right_column, std::move(copy_rows));
}

struct PasteMenu : public QMenu {
  QAction paste_over_action;
  QAction paste_into_start_action;
  QAction paste_after_action;

  explicit PasteMenu(SongWidget& song_widget);
};
