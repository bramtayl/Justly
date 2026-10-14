#include "common/helpers.hpp"

#include <QtCore/QItemSelectionModel>
#include <QtGui/QGuiApplication>
#include <QtGui/QUndoStack>
#include <QtWidgets/QMessageBox>

auto to_int(const double value) -> int {
  return static_cast<int>(std::round(value));
}

auto get_clipboard() -> QClipboard& {
  return get_reference(QGuiApplication::clipboard());
}

void clear_and_clean(QUndoStack& undo_stack) {
  undo_stack.clear();
  undo_stack.setClean();
}

void maybe_push(QUndoStack& undo_stack, QUndoCommand* const undo_command) {
  if (undo_command != nullptr) {
    undo_stack.push(undo_command);
  }
}

auto get_number_of_rows(const QItemSelectionRange& range) -> int {
  Q_ASSERT(range.isValid());
  return range.bottom() - range.top() + 1;
}

auto make_range(QAbstractItemModel& model, const int first_row_number,
                const int number_of_rows, const int left_column,
                const int right_column) -> QItemSelectionRange {
  return {model.index(first_row_number, left_column),
          model.index(first_row_number + number_of_rows - 1, right_column)};
}

auto get_share_folder() -> QDir {
  QDir folder(QCoreApplication::applicationDirPath());
  folder.cdUp();
  if (folder.dirName() == "Contents") {
    folder.cd("Resources");
  } else {
    folder.cd("share");
  }
  return folder;
}

auto get_share_file(const char* file_name) -> std::string {
  static const auto share_folder = get_share_folder();
  const auto result_file = share_folder.filePath(file_name);
  if (!QFile::exists(result_file)) {
    // Q_ASSERT compiles out in release builds, so a broken/incomplete
    // installation missing a bundled resource (xsd schema, icon,
    // soundfont) would otherwise silently hand the caller a nonexistent
    // path instead of failing here; there's no valid path to hand back,
    // so report it to the user and exit rather than throwing
    QMessageBox::critical(
        nullptr, QObject::tr("Missing resource file"),
        QObject::tr("Missing bundled resource file: %1").arg(result_file));
    std::exit(EXIT_FAILURE);  // NOLINT(concurrency-mt-unsafe)
  }
  return result_file.toStdString();
}
