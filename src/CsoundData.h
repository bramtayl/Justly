#pragma once

#include <csound/csound.h>  // for CSOUND
#include <stdint.h>         // for uintptr_t

#include <string>  // for string

uintptr_t csound_thread(void *csound_data_pointer);

class CsoundData {
 public:
  CSOUND *csound_object_pointer = nullptr;
  bool is_running = false;
  bool should_stop_running = false;
  bool should_start_playing = false;
  bool is_playing = false;
  bool should_stop_playing = false;
  void *thread_id;

  CsoundData();
  ~CsoundData();
  void start_song(std::string &csound_file);
  void stop_song();
  void run_backend();
};