#pragma once

#include <QUndoStack>
#include <QtGlobal>

struct SongEditor;

struct NotesToChords : public QUndoCommand {
  SongEditor *const song_editor_pointer;
  qsizetype chord_number;

  explicit NotesToChords(SongEditor *song_editor_pointer_input,
                          qsizetype chord_number_input, QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
