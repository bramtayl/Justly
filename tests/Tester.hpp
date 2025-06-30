#pragma once

#include <QtCore/QObject>

#include "widgets/SongEditor.hpp"

struct Tester : public QObject {
  Q_OBJECT
public:
  SongEditor song_editor;
  QDir test_dir = []() {
    QDir test_dir(QCoreApplication::applicationDirPath());
    test_dir.cdUp();
    test_dir.cd("share");
    return test_dir;
  }();
  bool waiting_for_message = false;
private slots:
  void run_tests();
};