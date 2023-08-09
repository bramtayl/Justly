#pragma once

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qobject.h>             // for QObject
#include <qstring.h>             // for QString
#include <qtemporaryfile.h>
#include <qtmetamacros.h>        // for Q_OBJECT, slots
#include <qvariant.h>            // for QVariant

#include "Editor.h"  // for Editor
#include "Player.h"
#include "Song.h"
class TreeNode;

class Tester : public QObject {
  Q_OBJECT
 public:

  QTemporaryFile main_file;

  Song song;
  Player player = Player(song);
  
  Editor editor = Editor(song, player);
  QModelIndex root_index = QModelIndex();
  QModelIndex first_chord_symbol_index;
  QModelIndex first_note_symbol_index;
  QModelIndex third_chord_symbol_index;
  QModelIndex second_chord_symbol_index;
  QModelIndex first_note_instrument_index;
  
  TreeNode *first_chord_node_pointer;
  TreeNode *first_note_node_pointer;
  TreeNode* third_chord_node_pointer;
  bool loaded_correctly = false;

  explicit Tester();

  [[nodiscard]] auto get_data(int row, int column, QModelIndex &parent_index)
      -> QVariant;
  [[nodiscard]] auto get_color(int row, int column, QModelIndex &parent_index)
      -> QVariant;
  [[nodiscard]] auto set_data(int row, int column, QModelIndex &parent_index,
                              const QVariant &new_value) -> bool;
  [[nodiscard]] auto get_column_heading(int column) const -> QVariant;
  void select_index(QModelIndex index);
  void select_indices(QModelIndex first_index, QModelIndex last_index);
  void clear_selection();
  auto dismiss_load_text(const QString &text) -> bool;
  void dismiss_paste(int first_index, const QString &paste_text, const QModelIndex &parent_index);
  void dismiss_save(const QString& filename);
  void dismiss_save_as(const QString& filename);
  void dismiss_open(const QString& filename);

 private slots:
  static void dismiss_messages();
  void initTestCase() const;

  void test_column_headers();
  void test_insert_delete();
  void test_colors();
  void test_copy_paste();
  void test_get_value();
  void test_set_value();
  void test_play();
  void test_flags();
  void test_tree();
  void test_controls();
  void test_select();
  void test_json();
  void test_view();
  void test_delegates();
  void test_io();
};
