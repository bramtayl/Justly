#pragma once

#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtWidgets/QMenu>

#include "actions/InsertRemoveRows.hpp"
#include "other/Cells.hpp"
#include "widgets/SongWidget.hpp"
#include "xml/XMLDocument.hpp"

[[nodiscard]] static auto get_mime_description(const QString &mime_type) {
  Q_ASSERT(mime_type.isValidUtf16());
  if (mime_type == Chord::get_cells_mime()) {
    return QObject::tr("chords cells");
  }
  if (mime_type == PitchedNote::get_cells_mime()) {
    return QObject::tr("pitched notes cells");
  }
  if (mime_type == UnpitchedNote::get_cells_mime()) {
    return QObject::tr("unpitched notes cells");
  }
  return mime_type;
}

[[nodiscard]] static inline auto get_clipboard() -> auto & {
  return get_reference(QGuiApplication::clipboard());
}

template <RowInterface SubRow>
[[nodiscard]] static auto
parse_clipboard(QWidget &parent,
                const int max_rows = std::numeric_limits<int>::max())
    -> std::optional<Cells<SubRow>> {
  const auto &mime_data = get_reference(get_clipboard().mimeData());
  const auto *mime_type = SubRow::get_cells_mime();
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

  const auto &copied_text = mime_data.data(mime_type).toStdString();
  XMLDocument document(xmlReadMemory(copied_text.c_str(),
                                     static_cast<int>(copied_text.size()),
                                     nullptr, nullptr, 0));
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

  auto &root = get_root(document);
  QList<SubRow> new_rows;
  auto left_column = 0;
  auto right_column = 0;

  auto *field_pointer = xmlFirstElementChild(&root);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto name = get_xml_name(field_node);
    if (name == "left_column") {
      left_column = xml_to_int(field_node);
    } else if (name == "right_column") {
      right_column = xml_to_int(field_node);
    } else if (name == "rows") {
      auto counter = 1;
      auto *xml_row_pointer = xmlFirstElementChild(&field_node);
      while (xml_row_pointer != nullptr && counter <= max_rows) {
        SubRow child_row;
        child_row.from_xml(get_reference(xml_row_pointer));
        new_rows.push_back(std::move(child_row));
        xml_row_pointer = xmlNextElementSibling(xml_row_pointer);
        counter++;
      }
    } else {
      Q_ASSERT(false);
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
  return Cells(left_column, right_column, std::move(new_rows));
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_paste_insert_command(QWidget &parent, RowsModel<SubRow> &rows_model,
                          const int row_number) -> QUndoCommand * {
  const auto maybe_cells = parse_clipboard<SubRow>(parent);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto &cells = maybe_cells.value();
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, std::move(cells.rows), cells.left_column,
      cells.right_column, false);
}

static void add_paste_insert(SongWidget &song_widget, const int row_number) {
  auto &switch_column = song_widget.switch_column;
  auto &chords_table = switch_column.chords_table;
  auto &pitched_notes_table = switch_column.pitched_notes_table;
  auto &unpitched_notes_table = switch_column.unpitched_notes_table;

  QUndoCommand *undo_command = nullptr;
  switch (switch_column.current_row_type) {
  case chord_type:
    undo_command = make_paste_insert_command(
        chords_table, chords_table.chords_model, row_number);
    break;
  case pitched_note_type:
    undo_command = make_paste_insert_command(
        pitched_notes_table, pitched_notes_table.pitched_notes_model,
        row_number);
    break;
  case unpitched_note_type:
    undo_command = make_paste_insert_command(
        unpitched_notes_table, unpitched_notes_table.unpitched_notes_model,
        row_number);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  if (undo_command == nullptr) {
    return;
  }
  song_widget.undo_stack.push(undo_command);
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_paste_cells_command(QWidget &parent, const int first_row_number,
                         RowsModel<SubRow> &rows_model) -> QUndoCommand * {
  auto &rows = rows_model.get_rows();
  auto maybe_cells = parse_clipboard<SubRow>(
      parent, static_cast<int>(rows.size()) - first_row_number);
  if (!maybe_cells.has_value()) {
    return nullptr;
  }
  auto &cells = maybe_cells.value();
  auto &copy_rows = cells.rows;
  const auto number_copied = static_cast<int>(copy_rows.size());
  return make_set_cells_command(rows_model, first_row_number, number_copied,
                                cells.left_column, cells.right_column,
                                std::move(copy_rows));
}

struct PasteMenu : public QMenu {
  QAction paste_over_action = QAction(PasteMenu::tr("&Over"));
  QAction paste_into_action = QAction(PasteMenu::tr("&Into start"));
  QAction paste_after_action = QAction(PasteMenu::tr("&After"));

  explicit PasteMenu(SongWidget &song_widget) : QMenu(PasteMenu::tr("&Paste")) {
    add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
    add_menu_action(*this, paste_into_action);
    add_menu_action(*this, paste_after_action, QKeySequence::StandardKey(),
                    false);

    QObject::connect(
        &paste_over_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_column = song_widget.switch_column;
          auto &chords_table = switch_column.chords_table;
          auto &pitched_notes_table = switch_column.pitched_notes_table;
          auto &unpitched_notes_table = switch_column.unpitched_notes_table;

          const auto first_row_number = get_only_range(switch_column).top();

          QUndoCommand *undo_command = nullptr;
          switch (switch_column.current_row_type) {
          case chord_type:
            undo_command = make_paste_cells_command(
                chords_table, first_row_number, chords_table.chords_model);
            break;
          case pitched_note_type:
            undo_command = make_paste_cells_command(
                pitched_notes_table, first_row_number,
                pitched_notes_table.pitched_notes_model);
            break;
          case unpitched_note_type:
            undo_command = make_paste_cells_command(
                unpitched_notes_table, first_row_number,
                unpitched_notes_table.unpitched_notes_model);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          if (undo_command == nullptr) {
            return;
          }
          song_widget.undo_stack.push(undo_command);
        });

    QObject::connect(&paste_into_action, &QAction::triggered, this,
                     [&song_widget]() { add_paste_insert(song_widget, 0); });

    QObject::connect(&paste_after_action, &QAction::triggered, this,
                     [&song_widget]() {
                       add_paste_insert(song_widget, get_next_row(song_widget));
                     });
  }
};