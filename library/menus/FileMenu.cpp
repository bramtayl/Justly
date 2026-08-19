#include "menus/FileMenu.hpp"

#include <QtCore/qobjectdefs.h>

#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QtAssert>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QFileDialog>

#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"

auto make_file_dialog(SongWidget& song_widget, const char* const caption,
                      const QString& filter,
                      const QFileDialog::AcceptMode accept_mode,
                      const QString& suffix,
                      const QFileDialog::FileMode file_mode) -> QFileDialog& {
  Q_ASSERT(filter.isValidUtf16());
  Q_ASSERT(suffix.isValidUtf16());
  auto& dialog =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_widget, SongWidget::tr(caption),
                        song_widget.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

auto get_selected_file(SongWidget& song_widget, const QFileDialog& dialog)
    -> QString {
  song_widget.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

FileMenu::FileMenu(SongWidget& song_widget)
    : QMenu(FileMenu::tr("&File")),
      save_action(FileMenu::tr("&Save")),
      open_action(FileMenu::tr("&Open")),
      save_as_action(FileMenu::tr("&Save As...")),
      import_action(FileMenu::tr("&Import MusicXML")),
      export_action(FileMenu::tr("&Export recording")),
      export_midi_action(FileMenu::tr("Export &MIDI")) {
  auto& save_action_ref = this->save_action;
  add_menu_action(*this, open_action, QKeySequence::Open);
  add_menu_action(*this, import_action, QKeySequence::UnknownKey, true);
  addSeparator();
  add_menu_action(*this, save_action, QKeySequence::Save, false);
  add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
  add_menu_action(*this, export_action);
  add_menu_action(*this, export_midi_action);

  QObject::connect(
      &song_widget.undo_stack, &QUndoStack::cleanChanged, this,
      [&save_action_ref, &song_widget]() -> auto {
        save_action_ref.setEnabled(!song_widget.undo_stack.isClean() &&
                                   !song_widget.current_file.isEmpty());
      });

  // open_action/import_action are wired externally in SongEditor.hpp,
  // which is the first header up the include chain with access to both
  // SongMenuBar and this widget's PianoRollWidget, needed to refresh the
  // view menu and piano roll after replacing the song wholesale

  QObject::connect(&save_action, &QAction::triggered, this,
                   [&song_widget]() -> auto {
                     save_as_file(song_widget, song_widget.current_file);
                   });

  QObject::connect(
      &save_as_action, &QAction::triggered, this, [&song_widget]() -> auto {
        auto& dialog = make_file_dialog(
            song_widget, "Save As — Justly", "XML file (*.xml)",
            QFileDialog::AcceptSave, ".xml", QFileDialog::AnyFile);

        if (dialog.exec() != 0) {
          save_as_file(song_widget, get_selected_file(song_widget, dialog));
        }
        dialog.deleteLater();
      });

  QObject::connect(
      &export_action, &QAction::triggered, this, [&song_widget]() -> auto {
        auto& dialog = make_file_dialog(
            song_widget, "Export — Justly", "WAV file (*.wav)",
            QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
        dialog.setLabelText(QFileDialog::Accept, "Export");
        if (dialog.exec() != 0) {
          export_to_file(song_widget, get_selected_file(song_widget, dialog));
        }
        dialog.deleteLater();
      });

  QObject::connect(
      &export_midi_action, &QAction::triggered, this, [&song_widget]() -> auto {
        auto& dialog = make_file_dialog(
            song_widget, "Export MIDI — Justly", "MIDI file (*.mid)",
            QFileDialog::AcceptSave, ".mid", QFileDialog::AnyFile);
        dialog.setLabelText(QFileDialog::Accept, "Export");
        if (dialog.exec() != 0) {
          export_midi_to_file(song_widget,
                              get_selected_file(song_widget, dialog));
        }
        dialog.deleteLater();
      });
}
