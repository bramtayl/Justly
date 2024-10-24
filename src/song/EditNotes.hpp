#pragma once

#include <QUndoStack>

struct SongEditor;

struct EditNotes : public QUndoCommand {
  SongEditor *const song_editor_pointer;
  int chord_number;

  explicit EditNotes(SongEditor *song_editor_pointer_input,
                     int chord_number_input,
                     QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
