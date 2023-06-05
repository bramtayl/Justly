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
  bool ready_to_start = true;
  void *const thread_id;
  std::mutex csound_mutex;
  std::condition_variable start_playing;
  std::condition_variable ready_to_start_signaller;
  std::condition_variable stop_running;
  std::condition_variable stop_playing;
  
  explicit CsoundData();
  ~CsoundData();
  void start_song(const QString &orchestra_text, const QString &score_text);
  void stop_song();
  void run_backend();
};