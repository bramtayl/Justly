#pragma once

#include <qjsonarray.h>
#include <qstring.h>     // for QString
#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include <memory>  // for unique_ptr
#include <vector>  // for vector

#include "StableIndex.h"  // for StableIndex
#include "TreeNode.h"     // for TreeNode

class ChordsModel;
class Editor;  // lines 12-12
class QModelIndex;

class Remove : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const int first_index;
  const int number_of_children;
  const StableIndex stable_parent_index;
  std::vector<std::unique_ptr<TreeNode>> deleted_children;

  explicit Remove(ChordsModel &chords_model_input, int first_index_input,
                  int number_of_rows_input, const QModelIndex &parent_index_input,
                  QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};

class Insert : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const int first_index;
  const QJsonArray insertion;
  const StableIndex stable_parent_index;

  Insert(ChordsModel &chords_model_input, int first_index_input,
         QJsonArray insertion_input,
         const QModelIndex &parent_index_input,
         QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};


class InsertEmptyRows : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const int first_index;
  const int number_of_children;
  const StableIndex stable_parent_index;

  explicit InsertEmptyRows(ChordsModel &chords_model_input, int first_index_input,
                           int number_of_rows_input,
                           const QModelIndex &parent_index_input,
                           QUndoCommand *parent_pointer_input = nullptr);

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
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
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
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
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
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};

class StartingInstrumentChange : public QUndoCommand {
 public:
  Editor &editor;
  const QString old_starting_instrument;
  QString new_starting_instrument;
  bool first_time = true;
  explicit StartingInstrumentChange(Editor &editor,
                                    QString new_starting_instrument_input);
  void undo() override;
  void redo() override;
  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand *next_command_pointer) -> bool override;
};

class SetData : public QUndoCommand {
 public:
  ChordsModel &chords_model;
  const StableIndex stable_parent_index;
  const QVariant old_value;
  const QVariant new_value;
  explicit SetData(ChordsModel &chords_model_input,
                      const QModelIndex &parent_index_input, QVariant new_value_input,
                      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
