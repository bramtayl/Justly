#include "menus/FileMenu.hpp"

#include "widgets/SwitchColumn.hpp"
#include "widgets/WindowBody.hpp"

auto make_file_dialog(WindowBody& window_body, const char* const caption,
                      const QString& filter,
                      const QFileDialog::AcceptMode accept_mode,
                      const QString& suffix,
                      const QFileDialog::FileMode file_mode) -> QFileDialog& {
  Q_ASSERT(filter.isValidUtf16());
  Q_ASSERT(suffix.isValidUtf16());
  auto& dialog =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&window_body, WindowBody::tr(caption),
                        window_body.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

auto get_selected_file(WindowBody& window_body, const QFileDialog& dialog)
    -> QString {
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
  auto& save_action_ref = this->save_action;
  add_menu_action(*this, open_action, QKeySequence::Open);
  add_menu_action(*this, import_action, QKeySequence::UnknownKey, true);
  addSeparator();
  add_menu_action(*this, save_action, QKeySequence::Save, false);
  add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
  add_menu_action(*this, export_action);

  QObject::connect(
      &window_body.undo_stack, &QUndoStack::cleanChanged, this,
      [&save_action_ref, &window_body]() -> auto {
        save_action_ref.setEnabled(!window_body.undo_stack.isClean() &&
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
        auto& dialog = make_file_dialog(
            window_body, "Save As — Justly", "XML file (*.xml)",
            QFileDialog::AcceptSave, ".xml", QFileDialog::AnyFile);

        if (dialog.exec() != 0) {
          save_as_file(window_body, get_selected_file(window_body, dialog));
        }
        dialog.deleteLater();
      });

  QObject::connect(
      &export_action, &QAction::triggered, this, [&window_body]() -> auto {
        auto& dialog = make_file_dialog(
            window_body, "Export — Justly", "WAV file (*.wav)",
            QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
        dialog.setLabelText(QFileDialog::Accept, "Export");
        if (dialog.exec() != 0) {
          export_to_file(window_body, get_selected_file(window_body, dialog));
        }
        dialog.deleteLater();
      });
}
