#pragma once

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QObject>
#include <QString>
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
#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/Note.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordField.hpp"
#include "justly/TreeLevel.hpp"

[[nodiscard]] auto get_level(QModelIndex index) -> TreeLevel;

[[nodiscard]] auto make_validator(const std::string &title, nlohmann::json json)
    -> nlohmann::json_schema::json_validator;

[[nodiscard]] auto
validate_json(QWidget *parent_pointer, const nlohmann::json &copied,
              const nlohmann::json_schema::json_validator &validator) -> bool;

void JUSTLY_EXPORT copy_text(const std::string &text, const QString &mime_type);

class JUSTLY_EXPORT ChordsModel : public QAbstractItemModel {
  Q_OBJECT

private:
  QWidget *const parent_pointer;
  QUndoStack *const undo_stack_pointer;

  [[nodiscard]] auto
  index_less_than_equals(const QModelIndex &index_1,
                         const QModelIndex &index_2) const -> bool;

  void check_chord_number(size_t chord_number) const;
  void check_chord_number_end(size_t chord_number) const;

  [[nodiscard]] auto get_chord(size_t chord_number) -> Chord &;
  [[nodiscard]] auto get_const_note_chord_pointer(
      const QModelIndex &index) const -> const NoteChord *;

  [[nodiscard]] auto
  top_left_index(const QItemSelection &item_selection) const -> QModelIndex;
  [[nodiscard]] auto
  bottom_right_index(const QItemSelection &item_selection) const -> QModelIndex;
  
  [[nodiscard]] auto get_number_of_additional_note_chords(
      size_t first_chord_number) const -> size_t;
  [[nodiscard]] auto
  get_bottom_right_index_from_chord(size_t chord_number,
                                    NoteChordField note_chord_field,
                                    size_t skip_rows) const -> QModelIndex;
  [[nodiscard]] auto
  get_bottom_right_index(const QModelIndex &first_row_index,
                         NoteChordField note_chord_field,
                         size_t skip_rows) const -> QModelIndex;

  [[nodiscard]] auto parse_clipboard(const QString &mime_type) -> bool;
  void add_cell_change(const QModelIndex &index, const QVariant &new_value);
  void add_cell_changes(const QModelIndex &top_left,
                        const QModelIndex &bottom_right,
                        const std::vector<NoteChord> &old_note_chord_templates,
                        const std::vector<NoteChord> &new_note_chord_templates);

  void copy_note_chord_templates_from_chord(
      size_t first_chord_number, const QModelIndex &bottom_right,
      std::vector<NoteChord> *note_chords_pointer) const;
  [[nodiscard]] auto copy_note_chord_templates(
      const QModelIndex &top_left,
      const QModelIndex &bottom_right) const -> std::vector<NoteChord>;

  void replace_note_cells(size_t chord_number, size_t first_note_number,
                          NoteChordField left_note_chord_field,
                          NoteChordField right_note_chord_field,
                          const std::vector<NoteChord> &note_chord_templates,
                          size_t first_template_number, size_t write_number);

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
  void replace_chords_cells(size_t first_chord_number,
                            NoteChordField left_note_chord_field,
                            NoteChordField right_note_chord_field,
                            const std::vector<NoteChord> &note_chord_templates,
                            size_t first_template_number = 0);
  void replace_notes_cells(size_t first_chord_number, size_t first_note_number,
                           NoteChordField left_note_chord_field,
                           NoteChordField right_note_chord_field,
                           const std::vector<NoteChord> &note_chord_templates);

  [[nodiscard]] auto
  copy_chords_to_json(size_t first_chord_number,
                      size_t number_of_chords) const -> nlohmann::json;

  void paste_rows(size_t first_child_number, const QModelIndex &parent_index);

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

  void copy_selected(const QItemSelection &selection) const;
  void delete_selected(const QItemSelection &selection);
  void paste_cells_or_after(const QItemSelection &selection);
};
