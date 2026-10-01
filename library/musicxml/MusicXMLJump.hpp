#pragma once

#include <QtCore/QList>
#include <QtCore/QString>

enum class JumpType { da_capo, dal_segno, to_coda, fine };

// a direction to continue somewhere else once a measure ends; a fine
// continues to the end of the score
struct MusicXMLJump {
  JumpType type = JumpType::da_capo;
  // the name of the segno or coda to jump to
  QString target;
  // the times through the measure the jump is taken on. If empty, a da capo
  // or dal segno is taken the first time, and a to coda or fine only after
  // a da capo or dal segno
  QList<int> times;
};
