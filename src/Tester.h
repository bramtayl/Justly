#pragma once

#include <qobject.h>      // for QObject
#include <qstring.h>      // for QString
#include <qtmetamacros.h> // for Q_OBJECT, slots
#include <qvariant.h>     // for QVariant

#include "Editor.h" // for Editor

class QModelIndex; // lines 10-10

class Tester : public QObject {
  Q_OBJECT
public:
  Editor editor;

  [[nodiscard]] auto get_data(int row, int column, QModelIndex &parent_index)
      -> QVariant;
  [[nodiscard]] auto get_color(int row, int column, QModelIndex &parent_index)
      -> QVariant;
  [[nodiscard]] auto set_data(int row, int column, QModelIndex &parent_index,
                              const QVariant &new_value) -> bool;
  [[nodiscard]] auto get_column_heading(int column) const -> QVariant;
  void select_indices(QModelIndex first_index, QModelIndex last_index);
  void clear_indices(QModelIndex first_index, QModelIndex last_index);
  void load_text(const QString &text);
  void dismiss_load_text(const QString &text);
  void dismiss_save_orchestra_text();

private slots:
  void initTestCase();
  void test_column_headers();
  void test_insert_delete();
  void test_colors();
  void test_get_value();
  void test_set_value();
  void test_play();
  void test_flags();
  void test_tree();
  void test_save() const;
  void test_orchestra();
  void test_sliders();
  static void dismiss_messages();
  void test_select();
  void test_json();
  void test_view();
};
