#include "CsoundData.h"

#include <QtCore/qglobal.h>  // for qCritical
#include <bits/chrono.h>     // for milliseconds
#include <csound/csound.h>   // for csoundReset, csoundCompile, csoundCreate
#include <qbytearray.h>      // for QByteArray

#include <thread>  // for sleep_for
#include <utility>
#include <vector>  // for vector

const auto SLEEP_TIME = 100;

CsoundData::CsoundData()
    : csound_object_pointer(csoundCreate(nullptr)),
      thread_id(csoundCreateThread(csound_thread, (void *)this)){};

CsoundData::~CsoundData() {
  stop_song();
  if (is_running) {
    should_stop_running = true;
    while (is_running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    }
    should_stop_running = false;
  }
  csoundDestroy(csound_object_pointer);
};

void CsoundData::start_song(std::vector<const char *> csound_arguments) {
  const auto compile_error_code =
      csoundCompile(csound_object_pointer, static_cast<int>(csound_arguments.size()), csound_arguments.data());
  if (compile_error_code != 0) {
    qCritical("Can't compile csound document!");
  }
  should_start_playing = true;
  while (!(is_playing)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
  }
  should_start_playing = false;
}

void CsoundData::stop_song() {
  if (is_playing) {
    should_stop_playing = true;
    while (is_playing) {
      std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    }
    should_stop_playing = false;
  }
};

void CsoundData::run_backend() {
  is_running = true;
  while (true) {
    if (should_stop_running) {
      is_running = false;
      break;
    }
    if (should_start_playing) {
      is_playing = true;
      while (true) {
        if (should_stop_playing) {
          break;
        }
        const auto is_finished = csoundPerformKsmps(csound_object_pointer);
        if (is_finished != 0) {
          break;
        }
      }
      csoundReset(csound_object_pointer);
      is_playing = false;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    }
  }
}

auto csound_thread(void *csound_data_pointer) -> uintptr_t {
  (static_cast<CsoundData *>(csound_data_pointer))->run_backend();
  return 1;
}