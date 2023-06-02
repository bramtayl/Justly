#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qundostack.h>          // for QUndoCommand
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "TreeNode.h"  // for TreeNode
class Editor;
class Song;  // lines 11-11

class CellChange : public QUndoCommand {
 public:
  Song &song;
  const QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;
  CellChange(Song &song_input, const QModelIndex &index_input,
             QVariant new_value_input,
             QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class Remove : public QUndoCommand {
 public:
  Song &song;
  const int position;
  const size_t rows;
  const QModelIndex parent_index;
  std::vector<std::unique_ptr<TreeNode>> deleted_rows;

  Remove(Song &song_input, int position_input, size_t rows_input,
         const QModelIndex &parent_index_input,
         QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class Insert : public QUndoCommand {
 public:
  Song &song;
  const int position;
  const size_t rows;
  std::vector<std::unique_ptr<TreeNode>> inserted;
  const QModelIndex parent_index;

  Insert(Song &song_input, int position_input,
         std::vector<std::unique_ptr<TreeNode>> &copied,
         const QModelIndex &parent_index_input,
         QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class InsertEmptyRows : public QUndoCommand {
 public:
  Song &song;
  const int position;
  const int rows;
  const QModelIndex parent_index;

  InsertEmptyRows(Song &song_input, int position_input, int rows_input,
                  const QModelIndex &parent_index_input,
                  QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class FrequencyChange : public QUndoCommand {
 public:
  Editor &editor;
  const int old_value;
  const int new_value;
  bool first_time = true;

  FrequencyChange(Editor &editor, int new_value_input);
  void undo() override;
  void redo() override;
};

class VolumeChange : public QUndoCommand {
 public:
  Editor &editor;
  const int old_value;
  const int new_value;
  bool first_time = true;

  VolumeChange(Editor &editor_input, int new_value);
  void undo() override;
  void redo() override;
};

class TempoChange : public QUndoCommand {
 public:
  Editor &editor;
  const int old_value;
  const int new_value;
  bool first_time = true;

  TempoChange(Editor &editor_input, int new_value);
  void undo() override;
  void redo() override;
};

class OrchestraChange : public QUndoCommand {
  public:
    Editor &editor;
    const QString old_text;
    const QString new_text;
    bool first_time = true;
    OrchestraChange(Editor &editor, const QString &old_text, const QString &new_text);
    void undo() override;
    void redo() override;
};

class DefaultInstrumentChange : public QUndoCommand {
  public:
    Editor &editor;
    const QString old_text;
    const QString new_text;
    bool first_time = true;
    DefaultInstrumentChange(Editor &editor, const QString &old_text, const QString &new_text);
    void undo() override;
    void redo() override;
};
