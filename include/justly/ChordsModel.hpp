#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <cstddef>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "justly/Chord.hpp"
#include "justly/DataType.hpp"
#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/TreeLevel.hpp"

[[nodiscard]] auto get_level(QModelIndex index) -> TreeLevel;

[[nodiscard]] auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;
[[nodiscard]] auto validate_type(QWidget *parent_pointer,
                                 const nlohmann::json &copied,
                                 DataType data_type) -> bool;

class JUSTLY_EXPORT ChordsModel : public QAbstractItemModel {
  Q_OBJECT

private:
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;

  void check_chord_number(size_t chord_number) const;
  void check_chord_number_end(size_t chord_number) const;

  [[nodiscard]] auto get_chord(size_t chord_number) -> Chord &;
  [[nodiscard]] auto get_const_note_chord_pointer(
      const QModelIndex &index) const -> const NoteChord *;

  [[nodiscard]] auto parse_clipboard(DataType data_type) -> bool;
  void add_cell_change(const QModelIndex &index, const QVariant &new_value);

public:
  std::vector<Chord> chords;
  nlohmann::json clipboard;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto get_chord_index(
      size_t chord_number,
      NoteChordField note_chord_field = type_column) const -> QModelIndex;
  [[nodiscard]] auto get_note_index(
      size_t chord_number, size_t note_number,
      NoteChordField note_chord_field = type_column) const -> QModelIndex;
  [[nodiscard]] auto
  get_const_chord(size_t chord_number) const -> const Chord &;

  [[nodiscard]] auto
  rowCount(const QModelIndex &parent_index) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;
  [[nodiscard]] auto
  parent(const QModelIndex &index) const -> QModelIndex override;
  [[nodiscard]] auto
  index(int child_number, int column,
        const QModelIndex &parent_index) const -> QModelIndex override;

  [[nodiscard]] auto headerData(int section, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
  auto insertRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;
  auto removeRows(int first_child_number, int number_of_children,
                  const QModelIndex &parent_index) -> bool override;

  void set_chord_cell(size_t chord_number, NoteChordField note_chord_field,
                      const QVariant &new_value);
  void set_note_cell(size_t chord_number, size_t note_number,
                     NoteChordField note_chord_field,
                     const QVariant &new_value);

  [[nodiscard]] auto
  copy_chords_to_json(size_t first_chord_number,
                      size_t number_of_chords) const -> nlohmann::json;

  [[nodiscard]] auto copy_cell(const QModelIndex &index) const -> bool;
  [[nodiscard]] auto paste_cell(const QModelIndex &index) -> bool;

  void copy_rows(size_t first_child_number, size_t number_of_children,
                 const QModelIndex &parent_index) const;
  [[nodiscard]] auto paste_rows(size_t first_child_number,
                                const QModelIndex &parent_index) -> bool;

  void insert_chords(size_t first_chord_number,
                     const std::vector<Chord> &new_chords);
  void insert_json_chords(size_t first_chord_number,
                          const nlohmann::json &json_chords);
  void remove_chords(size_t first_chord_number, size_t number_of_chords);
  void delete_all_chords();

  void insert_notes(size_t chord_number, size_t first_note_number,
                    const std::vector<Note> &new_notes);
  void insert_json_notes(size_t chord_number, size_t first_note_number,
                         const nlohmann::json &json_notes);
  void remove_notes(size_t chord_number, size_t first_note_number,
                    size_t number_of_notes);
};
