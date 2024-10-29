#pragma once

#include <QObject>
#include <QString>
#include <vector>

class QModelIndex;
struct SongEditor;

struct BadPasteRow {
  const QString copied;
  const QString mime_type;
  const QString error_message;
};

struct Tester : public QObject {
  Q_OBJECT

public:
  SongEditor *const song_editor_pointer;

  bool waiting_for_message = false;
  void close_message_later(const QString &expected_text);

  Tester();
  ~Tester() override;

  void test_bad_pastes(const QModelIndex &index,
                       const std::vector<BadPasteRow> &bad_paste_rows);

private slots:
  void initTestCase() const;

  void test_to_strings() const;

  void test_chords_count() const;
  void test_pitched_notes_count() const;
  void test_unpitched_notes_count() const;

  void test_back_to_chords() const;

  void test_number_of_chord_columns() const;
  void test_number_of_pitched_note_columns() const;
  void test_number_of_unpitched_note_columns() const;

  void test_gain_control() const;
  void test_starting_key_control() const;
  void test_starting_velocity_control() const;
  void test_starting_tempo_control() const;

  void test_row_headers() const;
  void test_chord_column_headers() const;
  void test_pitched_note_column_headers() const;
  void test_unpitched_note_column_headers() const;

  void test_chord_flags() const;
  void test_pitched_note_flags() const;
  void test_unpitched_note_flags() const;

  void test_chord_frequencies() const;
  void test_pitched_note_frequencies() const;
  void test_unpitched_status() const;

  void test_get_unsupported_chord_role() const;
  void test_get_unsupported_pitched_note_role() const;
  void test_get_unsupported_unpitched_note_role() const;

  void test_set_unsupported_chord_role() const;
  void test_set_unsupported_pitched_note_role() const;
  void test_set_unsupported_unpitched_note_role() const;

  void test_set_chord_values() const;
  void test_set_pitched_note_values() const;
  void test_set_unpitched_note_values() const;

  void test_delete_chord_cells() const;
  void test_delete_pitched_note_cells() const;
  void test_delete_unpitched_note_cells() const;

  void test_copy_paste_chord_cells() const;
  void test_copy_paste_pitched_note_cells() const;
  void test_copy_paste_unpitched_note_cells() const;

  void test_cut_paste_chord_cells() const;
  void test_cut_paste_pitched_note_cells() const;
  void test_cut_paste_unpitched_note_cells() const;

  void test_copy_paste_insert_chord() const;
  void test_copy_paste_insert_pitched_note() const;
  void test_copy_paste_insert_unpitched_note() const;

  void test_chord_insert_into() const;
  void test_pitched_note_insert_into() const;
  void test_unpitched_note_insert_into() const;

  void test_chord_insert_after() const;
  void test_pitched_note_insert_after() const;
  void test_unpitched_note_insert_after() const;

  void test_chord_remove_rows() const;
  void test_pitched_note_remove_rows() const;
  void test_unpitched_note_remove_rows() const;

  void test_bad_chord_pastes();
  void test_bad_pitched_note_pastes();
  void test_bad_unpitched_note_pastes();

  void test_too_loud();
  void test_too_many_channels();
  void test_missing_instruments();

  void test_chord_plays() const;
  void test_pitched_note_plays() const;
  void test_unpitched_note_plays() const;

  void test_save() const;
  void test_export() const;
  void test_broken_file();
  void test_open_empty() const;
};
