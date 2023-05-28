#pragma once

#include <csound/csound.h>  // for CSOUND
#include <qstring.h>        // for QString
#include <stdint.h>         // for uintptr_t

uintptr_t csound_thread(void *csound_data_pointer);

class CsoundData {
 public:
  CSOUND *const csound_object_pointer;
  bool is_running = false;
  bool should_stop_running = false;
  bool should_start_playing = false;
  bool is_playing = false;
  bool should_stop_playing = false;
  void *const thread_id;
  const std::string orchestra_file;

  explicit CsoundData(std::string orchestra_file);
  ~CsoundData();
  void start_song(const QString& csound_file);
  void stop_song();
  void run_backend();
};