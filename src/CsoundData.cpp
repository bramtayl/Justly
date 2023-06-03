#include "CsoundData.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/chrono.h>           // for milliseconds
#include <csound/csound.h>         // for csoundCompileOrc, csoundCreate
#include <qbytearray.h>            // for QByteArray
#include <qstring.h>               // for QString

#include <thread>  // for sleep_for

const auto SLEEP_TIME = 100;

CsoundData::CsoundData()
    : csound_object_pointer(csoundCreate(nullptr)),
      thread_id(csoundCreateThread(csound_thread, (void *)this)){};

CsoundData::~CsoundData() {
  stop_song();
  std::unique_lock<std::mutex> is_running_lock(is_running_mutex);
  if (is_running) {
    should_stop_running = true;
    is_running_condition_variable.wait(is_running_lock, [&]() {return !is_running;});
    should_stop_running = false;
  }
  csoundJoinThread(thread_id);
  csoundDestroy(csound_object_pointer);
};

void CsoundData::start_song(const QString &orchestra_text,
                            const QString &score_text) {
  csoundSetOption(csound_object_pointer, "--output=devaudio");
  csoundCompileOrc(csound_object_pointer, qUtf8Printable(orchestra_text));
  csoundReadScore(csound_object_pointer, qUtf8Printable(score_text));

  std::unique_lock<std::mutex> is_playing_lock(is_playing_mutex);
  should_start_playing = true;
  is_playing_condition_variable.wait(is_playing_lock, [&]() {return is_playing;});
  should_start_playing = false;
}

void CsoundData::stop_song() {
  std::unique_lock<std::mutex> is_playing_lock(is_playing_mutex);
  if (is_playing) {
    should_stop_playing = true;
    is_playing_condition_variable.wait(is_playing_lock, [&]() {return !is_playing;});
    should_stop_playing = false;
  }
};

void CsoundData::run_backend() {
  is_running = true;
  while (!should_stop_running) {
    if (should_start_playing) {
      csoundStart(csound_object_pointer); 
      {
        std::lock_guard<std::mutex> is_playing_lock(is_playing_mutex);
        is_playing = true;
        is_playing_condition_variable.notify_one();
      }
      while (!should_stop_playing && csoundPerformKsmps(csound_object_pointer) == 0) {
      }
      csoundReset(csound_object_pointer);
      {
        std::lock_guard<std::mutex> is_playing_lock(is_playing_mutex);
        is_playing = false;
        is_playing_condition_variable.notify_one();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
  }
  std::lock_guard<std::mutex> is_running_lock(is_running_mutex);
  is_running = false;
  is_running_condition_variable.notify_one();
}

auto csound_thread(void *csound_data_pointer) -> uintptr_t {
  (static_cast<CsoundData *>(csound_data_pointer))->run_backend();
  return 0;
}