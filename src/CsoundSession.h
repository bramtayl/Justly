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

  void load_orchestra(const QString &orchestra_text);
  void play(const QString &score_text);
  void stop_playing();
  void set_options();
};