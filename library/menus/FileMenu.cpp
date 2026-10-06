#include "menus/FileMenu.hpp"

#include "menus/MenuAction.hpp"
#include "sound/Playback.hpp"
#include "widgets/SongFile.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/WindowBody.hpp"

auto maybe_choose_file(WindowBody& window_body, const QString& caption,
                       const QString& filter,
                       const QFileDialog::AcceptMode accept_mode,
                       const QString& suffix, const QString& accept_label)
    -> std::optional<QString> {
  Q_ASSERT(filter.isValidUtf16());
  Q_ASSERT(suffix.isValidUtf16());
  QFileDialog dialog(&window_body, caption, window_body.current_folder, filter);

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(accept_mode == QFileDialog::AcceptOpen
                         ? QFileDialog::ExistingFile
                         : QFileDialog::AnyFile);
  if (!accept_label.isEmpty()) {
    dialog.setLabelText(QFileDialog::Accept, accept_label);
  }

  if (dialog.exec() == 0) {
    return std::nullopt;
  }
  window_body.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

FileMenu::FileMenu(WindowBody& window_body)
    : QMenu(FileMenu::tr("&File")),
      save_action(FileMenu::tr("&Save")),
      open_action(FileMenu::tr("&Open")),
      save_as_action(FileMenu::tr("&Save As...")),
      import_action(FileMenu::tr("&Import MusicXML")),
      export_action(FileMenu::tr("&Export recording")) {
  add_menu_action(*this, open_action, QKeySequence::Open);
  add_menu_action(*this, import_action);
  addSeparator();
  add_menu_action(*this, save_action, QKeySequence::Save, false);
  add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
  add_menu_action(*this, export_action);

  QObject::connect(
      &window_body.undo_stack, &QUndoStack::cleanChanged, this,
      [this, &window_body]() -> auto {
        save_action.setEnabled(!window_body.undo_stack.isClean() &&
                               !window_body.current_file.isEmpty());
      });

  // open_action/import_action are wired in MainWindow's constructor instead,
  // since refreshing the view menu and piano roll after replacing the song
  // wholesale needs SongMenuBar and PianoRollWidget, which this menu can't
  // reach

  QObject::connect(&save_action, &QAction::triggered, this,
                   [&window_body]() -> auto {
                     save_as_file(window_body, window_body.current_file);
                   });

  QObject::connect(
      &save_as_action, &QAction::triggered, this, [&window_body]() -> auto {
        const auto maybe_file = maybe_choose_file(
            window_body, FileMenu::tr("Save As — Justly"),
            FileMenu::tr("XML file (*.xml)"), QFileDialog::AcceptSave, ".xml");
        if (maybe_file.has_value()) {
          save_as_file(window_body, *maybe_file);
        }
      });

  QObject::connect(
      &export_action, &QAction::triggered, this, [&window_body]() -> auto {
        const auto maybe_file = maybe_choose_file(
            window_body, FileMenu::tr("Export — Justly"),
            FileMenu::tr("WAV file (*.wav)"), QFileDialog::AcceptSave, ".wav",
            FileMenu::tr("Export"));
        if (maybe_file.has_value()) {
          export_to_file(window_body.player, window_body.song, *maybe_file);
        }
      });
}
