#pragma once

#include <QtWidgets/QFileDialog>

#include "SongWidget.hpp"

[[nodiscard]] static auto make_file_dialog(
    SongWidget &song_widget, const char *const caption, const QString &filter,
    const QFileDialog::AcceptMode accept_mode, const QString &suffix,
    const QFileDialog::FileMode file_mode) -> QFileDialog & {
  Q_ASSERT(filter.isValidUtf16());
  Q_ASSERT(suffix.isValidUtf16());
  auto &dialog = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QFileDialog(&song_widget, SongWidget::tr(caption),
                        song_widget.current_folder, filter));

  dialog.setAcceptMode(accept_mode);
  dialog.setDefaultSuffix(suffix);
  dialog.setFileMode(file_mode);

  return dialog;
}

[[nodiscard]] static auto get_selected_file(SongWidget &song_widget,
                                            const QFileDialog &dialog) {
  song_widget.current_folder = dialog.directory().absolutePath();
  return get_only(dialog.selectedFiles());
}

struct FileMenu : public QMenu {
  QAction save_action = QAction(FileMenu::tr("&Save"));
  QAction open_action = QAction(FileMenu::tr("&Open"));
  QAction save_as_action = QAction(FileMenu::tr("&Save As..."));
  QAction import_action = QAction(FileMenu::tr("&Import MusicXML"));
  QAction export_action = QAction(FileMenu::tr("&Export recording"));

  explicit FileMenu(SongWidget &song_widget) : QMenu(FileMenu::tr("&File")) {
    auto &save_action_ref = this->save_action;
    add_menu_action(*this, open_action, QKeySequence::Open);
    add_menu_action(*this, import_action, QKeySequence::StandardKey(), true);
    addSeparator();
    add_menu_action(*this, save_action, QKeySequence::Save, false);
    add_menu_action(*this, save_as_action, QKeySequence::SaveAs);
    add_menu_action(*this, export_action);

    QObject::connect(&song_widget.undo_stack, &QUndoStack::cleanChanged, this,
                     [&save_action_ref, &song_widget]() {
                       save_action_ref.setEnabled(
                           !song_widget.undo_stack.isClean() &&
                           !song_widget.current_file.isEmpty());
                     });

    QObject::connect(&open_action, &QAction::triggered, this, [&song_widget]() {
      if (can_discard_changes(song_widget)) {
        auto &dialog = make_file_dialog(
            song_widget, "Open — Justly", "XML file (*.xml)",
            QFileDialog::AcceptOpen, ".xml", QFileDialog::ExistingFile);
        if (dialog.exec() != 0) {
          open_file_internal(song_widget, get_selected_file(song_widget, dialog));
        }
      }
    });

    QObject::connect(
        &import_action, &QAction::triggered, this, [&song_widget]() {
          if (can_discard_changes(song_widget)) {
            auto &dialog = make_file_dialog(
                song_widget, "Import MusicXML — Justly",
                "MusicXML file (*.musicxml)", QFileDialog::AcceptOpen,
                ".musicxml", QFileDialog::ExistingFile);
            if (dialog.exec() != 0) {
              import_musicxml_internal(song_widget,
                              get_selected_file(song_widget, dialog));
            }
          }
        });

    QObject::connect(&save_action, &QAction::triggered, this, [&song_widget]() {
      save_as_file_internal(song_widget, song_widget.current_file);
    });

    QObject::connect(
        &save_as_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Save As — Justly", "XML file (*.xml)",
              QFileDialog::AcceptSave, ".xml", QFileDialog::AnyFile);

          if (dialog.exec() != 0) {
            save_as_file_internal(song_widget, get_selected_file(song_widget, dialog));
          }
        });

    QObject::connect(
        &export_action, &QAction::triggered, this, [&song_widget]() {
          auto &dialog = make_file_dialog(
              song_widget, "Export — Justly", "WAV file (*.wav)",
              QFileDialog::AcceptSave, ".wav", QFileDialog::AnyFile);
          dialog.setLabelText(QFileDialog::Accept, "Export");
          if (dialog.exec() != 0) {
            export_to_file_internal(song_widget, get_selected_file(song_widget, dialog));
          }
        });
  }
};
