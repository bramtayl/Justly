#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qobject.h>             // for QObject
#include <qpointer.h>            // for QPointer
#include <qstring.h>             // for QString
#include <qtemporaryfile.h>
#include <qtimer.h>
#include <qtmetamacros.h>  // for Q_OBJECT, slots
#include <qvariant.h>      // for QVariant

#include "Editor.h"  // for Editor
#include "Song.h"
class TreeNode;

class Tester : public QObject {
  Q_OBJECT
 public:
  QTemporaryFile main_file;

  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();

  Song song;

  const std::unique_ptr<Editor> editor_pointer = std::make_unique<Editor>(song);
  QModelIndex root_index = QModelIndex();
  QModelIndex first_chord_symbol_index;
  QModelIndex first_note_symbol_index;
  QModelIndex third_chord_symbol_index;
  QModelIndex second_chord_symbol_index;
  QModelIndex first_note_instrument_index;

  TreeNode *first_chord_node_pointer = nullptr;
  TreeNode *first_note_node_pointer = nullptr;
  TreeNode *third_chord_node_pointer = nullptr;

  [[nodiscard]] auto get_data(int row, int column,
                              QModelIndex &parent_index) const -> QVariant;
  [[nodiscard]] auto get_color(int row, int column,
                               QModelIndex &parent_index) const -> QVariant;
  [[nodiscard]] auto set_data(int row, int column, QModelIndex &parent_index,
                              const QVariant &new_value) const -> bool;
  [[nodiscard]] auto get_column_heading(int column) const -> QVariant;
  void select_index(QModelIndex index) const;
  void select_indices(QModelIndex first_index, QModelIndex last_index) const;
  void clear_selection() const;
  void save_to(const QString &filename);

 private slots:
  void close_one_message();
  void initTestCase();

  void test_column_headers() const;
  void test_insert_delete();
  void test_colors();
  void test_copy_paste();
  void test_get_value();
  void test_set_value();
  void test_play() const;
  void test_flags() const;
  void test_tree();
  void test_controls();
  void test_select() const;
  void test_json();
  void test_view();
  void test_delegates();
  void test_io();
};
