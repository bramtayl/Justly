#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariant>
#include <Qt>
#include <cstddef>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "justly/Chord.hpp"
#include "justly/JUSTLY_EXPORT.hpp"
#include "justly/NoteChord.hpp"
#include "justly/NoteChordColumn.hpp"

struct CellIndex;
struct Note;
class QItemSelection;
class QUndoStack;
class QWidget;
struct RowRange;

[[nodiscard]] auto is_root_index(const QModelIndex &index) -> bool;
[[nodiscard]] auto valid_is_chord_index(const QModelIndex &index) -> bool;

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

  void check_chord_number(size_t chord_number) const;
  void check_chord_number_end(size_t chord_number) const;
  void check_chord_range(size_t first_chord_number,
                         size_t number_of_chords) const;

  [[nodiscard]] auto get_chord(size_t chord_number) -> Chord &;

  [[nodiscard]] auto
  get_number_of_rows_left(size_t first_chord_number) const -> size_t;
  [[nodiscard]] auto get_note_chords_from_ranges(
      const std::vector<RowRange> &row_ranges) const -> std::vector<NoteChord>;

  [[nodiscard]] auto parse_clipboard(const QString &mime_type) -> bool;

  void add_cell_change(const QModelIndex &index, const QVariant &new_value);
  void add_cell_changes(const std::vector<RowRange> &row_ranges,
                        NoteChordColumn left_field, NoteChordColumn right_field,
                        const std::vector<NoteChord> &new_note_chords);
  void add_row_ranges_from(std::vector<RowRange> *row_range_pointer,
                           size_t chord_number,
                           size_t number_of_note_chords) const;

public:
  std::vector<Chord> chords;
  nlohmann::json clipboard;

  explicit ChordsModel(QUndoStack *undo_stack_pointer_input,
                       QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto get_chord_index(
      size_t chord_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;
  [[nodiscard]] auto get_note_index(
      size_t chord_number, size_t note_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;
  [[nodiscard]] auto
  get_const_chord(size_t chord_number) const -> const Chord &;

  [[nodiscard]] auto
  rowCount(const QModelIndex &parent_index) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;
  [[nodiscard]] auto
  parent(const QModelIndex &index) const -> QModelIndex override;
  [[nodiscard]] auto
  index(int signed_first_child_number, int column,
        const QModelIndex &parent_index) const -> QModelIndex override;

  [[nodiscard]] auto headerData(int column, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
  [[nodiscard]] auto
  insertRows(int signed_first_child_number, int signed_number_of_children,
             const QModelIndex &parent_index) -> bool override;
  [[nodiscard]] auto
  removeRows(int signed_first_child_number, int signed_number_of_children,
             const QModelIndex &parent_index) -> bool override;

  void set_cell(const CellIndex &cell_index, const QVariant &new_value);
  void replace_cell_ranges(const std::vector<RowRange> &row_ranges,
                           NoteChordColumn left_field,
                           NoteChordColumn right_field,
                           const std::vector<NoteChord> &note_chords);

  [[nodiscard]] auto
  copy_chords_to_json(size_t first_chord_number,
                      size_t number_of_chords) const -> nlohmann::json;

  void paste_rows(size_t first_child_number, const QModelIndex &parent_index);

  void insert_chords(size_t first_chord_number,
                     const std::vector<Chord> &new_chords);
  void append_json_chords(const nlohmann::json &json_chords);
  void remove_chords(size_t first_chord_number, size_t number_of_chords);
  void delete_all_chords();

  void insert_notes(size_t chord_number, size_t first_note_number,
                    const std::vector<Note> &new_notes);
  void remove_notes(size_t chord_number, size_t first_note_number,
                    size_t number_of_notes);

  void copy_selected(const QItemSelection &selection) const;
  void paste_cells_or_after(const QItemSelection &selection);

  void delete_selected(const QItemSelection &selection);
};
