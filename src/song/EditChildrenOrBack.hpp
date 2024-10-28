#pragma once

#include <QUndoStack>

struct SongEditor;

struct EditChildrenOrBack : public QUndoCommand {
  SongEditor *const song_editor_pointer;
  int chord_number;
  bool is_notes;
  bool backwards;

  explicit EditChildrenOrBack(SongEditor *song_editor_pointer_input,
                              int chord_number_input, bool is_notes_input,
                              bool backwards_input,
                              QUndoCommand *parent_pointer_input = nullptr);
  void edit_children(bool should_edit_children) const;

  void undo() override;
  void redo() override;
};
