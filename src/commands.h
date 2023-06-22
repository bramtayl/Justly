#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qstring.h>             // for QString
#include <qundostack.h>          // for QUndoCommand
#include <qvariant.h>            // for QVariant

#include <cstddef>  // for size_t
#include <memory>   // for unique_ptr
#include <vector>   // for vector

#include "TreeNode.h"  // for TreeNode
class Editor;          // lines 12-12
class Song;            // lines 13-13
class ChordsModel;

class Remove : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const int position;
  const size_t rows;
  const QModelIndex parent_index;
  std::vector<std::unique_ptr<TreeNode>> deleted_rows;

  explicit Remove(ChordsModel &chords_model_input, int position_input, size_t rows_input,
                  const QModelIndex &parent_index_input,
                  QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class Insert : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const int position;
  const size_t rows;
  std::vector<std::unique_ptr<TreeNode>> inserted;
  const QModelIndex parent_index;

  Insert(ChordsModel &chords_model_input, int position_input,
         std::vector<std::unique_ptr<TreeNode>> &copied,
         const QModelIndex &parent_index_input,
         QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class InsertEmptyRows : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const int position;
  const int rows;
  const QModelIndex parent_index;

  explicit InsertEmptyRows(ChordsModel &chords_model_input, int position_input, int rows_input,
                           const QModelIndex &parent_index_input,
                           QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};

class StartingKeyChange : public QUndoCommand {
 public:
  Editor &editor;
  const double old_value;
  double new_value;
  bool first_time = true;

  explicit StartingKeyChange(Editor &editor, double new_value_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *other_pointer) -> bool override;
};

class StartingVolumeChange : public QUndoCommand {
 public:
  Editor &editor;
  const double old_value;
  double new_value;
  bool first_time = true;

  explicit StartingVolumeChange(Editor &editor_input, double new_value);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *other_pointer) -> bool override;
};

class StartingTempoChange : public QUndoCommand {
 public:
  Editor &editor;
  const double old_value;
  double new_value;
  bool first_time = true;

  explicit StartingTempoChange(Editor &editor_input, double new_value);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *other_pointer) -> bool override;
};

class OrchestraChange : public QUndoCommand {
 public:
  Editor &editor;
  const QString old_text;
  const QString new_text;
  const QString old_starting_instrument;
  const QString new_starting_instrument;
  bool first_time = true;
  explicit OrchestraChange(Editor &editor, QString old_text, QString new_text,
                           QString old_starting_instrument,
                           QString new_starting_instrument);
  void undo() override;
  void redo() override;
};

class StartingInstrumentChange : public QUndoCommand {
 public:
  Editor &editor;
  const QString old_text;
  const QString new_text;
  bool first_time = true;
  explicit StartingInstrumentChange(Editor &editor, QString old_text,
                                   QString new_text);
  void undo() override;
  void redo() override;
};

class CellChange : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const QModelIndex index;
  const QVariant old_value;
  const QVariant new_value;
  explicit CellChange(ChordsModel &chords_model_input, const QModelIndex &index_input,
                      QVariant new_value_input,
                      QUndoCommand *parent_input = nullptr);

  void undo() override;
  void redo() override;
};
