#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex, QAbstractItemModel
#include <qmimedata.h>
#include <qnamespace.h>    // for ItemFlags, Orientation
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qundostack.h>
#include <qvariant.h>  // for QVariant
#include <qwidget.h>

#include <cstddef>  // for size_t
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <string>                 // for string
#include <vector>                 // for vector

#include "justly/CellIndex.hpp"  // for CellIndex
#include "justly/Chord.hpp"      // for Chord
#include "justly/Note.hpp"       // for Note
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"    // for NoteChordField, symbol_column
#include "justly/TreeLevel.hpp"         // for TreeLevel
#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT

class JUSTLY_EXPORT ChordsModel : public QAbstractItemModel {
  Q_OBJECT

 private:
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;

  [[nodiscard]] auto make_parent_index(int parent_number) const -> QModelIndex;
  [[nodiscard]] auto to_cell_index(const QModelIndex &index) const -> CellIndex;
  [[nodiscard]] auto get_const_note_chord_pointer(int parent_number,
                                                  size_t child_number) const
      -> const NoteChord *;
  void throw_parse_error(const nlohmann::json::parse_error &parse_error);
  void add_cell_change(const QModelIndex &index, const QVariant &new_value);
  [[nodiscard]] auto validate(
      const nlohmann::json &copied,
      const nlohmann::json_schema::json_validator &validator) -> bool;
  void column_type_error(NoteChordField note_chord_field,
                         const std::string &type);
  void mime_type_error(const QMimeData *mime_pointer);
  [[nodiscard]] auto get_chord(int parent_number) -> Chord &;
  [[nodiscard]] auto get_const_chord(int parent_number) const -> const Chord &;
  void add_insert_json_change(size_t first_child_number,
                         const nlohmann::json &json_children,
                         int parent_number);
  void add_insert_remove_chords_change(size_t first_child_number,
                                       const std::vector<Chord> &new_chords,
                                       bool is_insert);
  void add_insert_remove_notes_change(size_t first_child_number,
                                      const std::vector<Note> &new_notes,
                                      int parent_number, bool is_insert);

 public:
  std::vector<Chord> chords;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);
  [[nodiscard]] auto get_index(
      int parent_number, size_t child_number,
      NoteChordField note_chord_field = symbol_column) const -> QModelIndex;

  void copy_rows(size_t first_child_number, size_t number_of_children,
                 int parent_number);
  void load_chords(const nlohmann::json &json_song);

  // overrided methods, generally take QModelIndex and are undoable
  [[nodiscard]] auto rowCount(const QModelIndex &parent_index) const
      -> int override;
  [[nodiscard]] auto columnCount(const QModelIndex &parent) const
      -> int override;

  [[nodiscard]] auto parent(const QModelIndex &index) const
      -> QModelIndex override;
  [[nodiscard]] auto index(int child_number, int column,
                           const QModelIndex &parent) const
      -> QModelIndex override;

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto flags(const QModelIndex &index) const
      -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;

  auto insertRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  auto removeRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value, int role)
      -> bool override;

  void insert_remove_json(size_t first_child_number,
                          const nlohmann::json &json_children,
                          int parent_number, bool should_insert);
  void set_cell(const CellIndex &cell_index, const QVariant &new_value);
  void insert_remove_chords(size_t first_child_number,
                            const std::vector<Chord> &new_chords,
                            bool should_insert);
  void insert_remove_notes(size_t first_child_number,
                           const std::vector<Note> &new_notes,
                           int parent_number, bool should_insert);

  void paste_rows(int first_child_number, const QModelIndex &parent_index);

  void paste_cell(const QModelIndex &index);
  void copy_cell(QModelIndex index);
};

[[nodiscard]] auto get_level(QModelIndex index) -> TreeLevel;

[[nodiscard]] auto to_parent_number(const QModelIndex &index) -> int;

void JUSTLY_EXPORT copy_text(const std::string &text, const char *mime_type);
