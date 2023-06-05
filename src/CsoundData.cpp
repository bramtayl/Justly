#include "CsoundData.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/chrono.h>           // for milliseconds
#include <csound/csound.hpp>         // for csoundCompileOrc, csoundCreate
#include <qbytearray.h>            // for QByteArray
#include <qstring.h>               // for QString
#include <QDebug>

CsoundData::CsoundData() {
};

CsoundData::~CsoundData() {
  stop_playing();
};

void CsoundData::play(const QString &orchestra_text,
                            const QString &score_text) {
  stop_playing();
  csound_object.Reset();
  csound_object.SetOption("--output=devaudio");
  csound_object.CompileOrc(qUtf8Printable(orchestra_text));
  csound_object.ReadScore(qUtf8Printable(score_text));
  csound_object.Start();
  thread_pointer = new CsoundPerformanceThread(&csound_object);
  thread_pointer -> Play();
}

void CsoundData::stop_playing() {
  if (thread_pointer != nullptr) {
    if (thread_pointer -> GetStatus() == 0) {
      thread_pointer -> Stop();
    }
    thread_pointer->Join();
  }
};
