#pragma once

#include <csound/csound.hpp>  // for CSOUND
#include <csound/csPerfThread.hpp>
#include <qstring.h>        // for QString

class CsoundData {
 public:
  Csound csound_object;
  CsoundPerformanceThread *thread_pointer = nullptr;
  
  explicit CsoundData();
  ~CsoundData();
  void play(const QString &orchestra_text, const QString &score_text);
  void stop_playing();

  CSOUND_PARAMS csound_parameters;
};