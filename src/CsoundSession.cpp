#include "CsoundSession.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/chrono.h>           // for milliseconds
#include <csound/csound.hpp>         // for csoundCompileOrc, csoundCreate
#include <qbytearray.h>            // for QByteArray
#include <qstring.h>               // for QString
#include <QDebug>

CsoundSession::CsoundSession() : Csound() {
  set_options();
  
};

void CsoundSession::set_options() {
  SetOption("--output=devaudio");
  SetOption("--messagelevel=16");

}

CsoundSession::~CsoundSession() {
  stop_playing();
};

void CsoundSession::load_orchestra(const QString &orchestra_text) {
  stop_playing();
  Reset();
  set_options();
  CompileOrc(qUtf8Printable(orchestra_text));
  Start();
}


void CsoundSession::play(const QString &score_text) {
  ReadScore(qUtf8Printable(score_text));
  Start();
  thread_pointer = new CsoundPerformanceThread(this);
  thread_pointer -> Play();
}

void CsoundSession::stop_playing() {
  if (thread_pointer != nullptr) {
    if (thread_pointer -> GetStatus() == 0) {
      thread_pointer -> Stop();
    }
    thread_pointer->Join();
  }
  thread_pointer = nullptr;
};
