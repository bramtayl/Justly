#pragma once

#include <csound/csound.h>  // for CSOUND
#include <qstring.h>        // for QString
#include <stdint.h>         // for uintptr_t
#include <mutex>
#include <condition_variable>

uintptr_t csound_thread(void *csound_data_pointer);

class CsoundData {
 public:
  CSOUND *const csound_object_pointer;
  bool is_running = false;
  bool should_run = true;
  bool should_play = false;
  bool is_playing = false;
  void *const thread_id;
  std::mutex csound_mutex;
  std::condition_variable play_or_abort_signal;
  std::condition_variable stop_signal;
  std::condition_variable ready_signal;
  
  explicit CsoundData();
  ~CsoundData();
  void play(const QString &orchestra_text, const QString &score_text);
  void stop_playing();
  void abort();
  void run_backend();
};