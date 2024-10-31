#pragma once

#include <QUndoStack>

struct SongEditor;

struct EditChildrenOrBack : public QUndoCommand {
  SongEditor& song_editor;
  int chord_number;
  bool is_pitched;
  bool backwards;

  explicit EditChildrenOrBack(SongEditor& song_editor_input,
                              int chord_number_input, bool is_notes_input,
                              bool backwards_input);
  void undo() override;
  void redo() override;
};
