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
  std::mutex should_play_mutex;
  std::mutex is_playing_mutex;
  std::mutex should_run_mutex;
  std::condition_variable should_start_playing_condition_variable;
  std::condition_variable is_playing_condition_variable;
  std::condition_variable should_stop_running_condition_variable;
  std::condition_variable should_stop_playing_condition_variable;
  
  explicit CsoundData();
  ~CsoundData();
  void start_song(const QString &orchestra_text, const QString &score_text);
  void stop_song();
  void run_backend();
};