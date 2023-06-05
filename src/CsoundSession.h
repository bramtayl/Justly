#pragma once

#include <csound/csound.hpp>  // for CSOUND
#include <csound/csPerfThread.hpp>
#include <qstring.h>        // for QString

class CsoundSession : public Csound {
 public:
  Csound csound_object;
  CsoundPerformanceThread *thread_pointer = nullptr;
  
  explicit CsoundSession();
  ~CsoundSession();
  void play(const QString &orchestra_text, const QString &score_text);
  void stop_playing();
  void set_options();
};