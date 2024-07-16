#pragma once

#include <QAbstractItemModel>
#include <QMimeData>
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
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/TreeLevel.hpp"
#include "justly/public_constants.hpp"

[[nodiscard]] auto get_level(QModelIndex index) -> TreeLevel;

[[nodiscard]] auto
validate(QWidget *parent_pointer, const nlohmann::json &copied,
         const nlohmann::json_schema::json_validator &validator) -> bool;

void JUSTLY_EXPORT copy_text(const std::string &text,
                             const std::string &mime_type);

class JUSTLY_EXPORT ChordsModel : public QAbstractItemModel {
  Q_OBJECT

private:
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;

  [[nodiscard]] auto
  make_chord_index(size_t chord_number) const -> QModelIndex;

  void check_chord_number(size_t chord_number) const;
  void check_new_chord_number(size_t chord_number) const;

  [[nodiscard]] auto get_chord(size_t chord_number) -> Chord&;

  [[nodiscard]] auto get_const_note_chord_pointer(
      const QModelIndex &index) const -> const NoteChord *;

  void mime_type_error(const QMimeData *mime_pointer);
  void throw_parse_error(const nlohmann::json::parse_error &parse_error);
  void column_type_error(NoteChordField note_chord_field,
                         const std::string &type);

  void add_cell_change(const QModelIndex &index, const QVariant &new_value);

public:
  std::vector<Chord> chords;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto get_index(
      size_t child_number, int parent_number = -1,
      NoteChordField note_chord_field = symbol_column) const -> QModelIndex;
  [[nodiscard]] auto get_const_chord(size_t chord_number) const -> const Chord&;

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
  void set_note_cell(size_t note_number, NoteChordField note_chord_field,
                     size_t chord_number, const QVariant &new_value);

  void copy_cell(const QModelIndex &index);
  void paste_cell(const QModelIndex &index);

  void insert_notes(size_t first_note_number,
                             const std::vector<Note> &new_notes,
                             size_t chord_number);
  void insert_chords(size_t first_chord_number,
                              const std::vector<Chord> &new_chords);
  void remove_notes(size_t first_note_number,
                             size_t number_of_notes, size_t chord_number);
  void remove_chords(size_t first_chord_number,
                              size_t number_of_chords);

  [[nodiscard]] auto
  json_copy_chords(size_t first_chord_number,
                 size_t number_of_chords) const -> nlohmann::json;

  void insert_json_chords(size_t first_chord_number,
                          const nlohmann::json &json_chords);
  void insert_json_notes(size_t first_note_number,
                         const nlohmann::json &json_notes,
                         size_t chord_number);
  void load_chords(const nlohmann::json &json_chords);

  void copy_rows(size_t first_child_number, size_t number_of_children,
                 const QModelIndex &parent_index) const;
  void paste_rows(size_t first_child_number, const QModelIndex &parent_index);
};
