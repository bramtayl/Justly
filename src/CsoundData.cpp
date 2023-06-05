#include "CsoundData.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/chrono.h>           // for milliseconds
#include <csound/csound.h>         // for csoundCompileOrc, csoundCreate
#include <qbytearray.h>            // for QByteArray
#include <qstring.h>               // for QString
#include <QDebug>

#include <thread>  // for sleep_for

const auto LONG_TIME = std::chrono::milliseconds(10);
const auto SHORT_TIME = std::chrono::nanoseconds(10);

CsoundData::CsoundData()
    : csound_object_pointer(csoundCreate(nullptr)),
      thread_id(csoundCreateThread(csound_thread, (void *)this)){};

CsoundData::~CsoundData() {
  stop_song();
  {
    std::lock_guard<std::mutex> csound_lock(csound_mutex);
    should_run = false;
    stop_running.notify_one();
  }
  csoundJoinThread(thread_id);
  csoundDestroy(csound_object_pointer);
};

void CsoundData::start_song(const QString &orchestra_text,
                            const QString &score_text) {
  csoundSetOption(csound_object_pointer, "--output=devaudio");
  csoundCompileOrc(csound_object_pointer, qUtf8Printable(orchestra_text));
  csoundReadScore(csound_object_pointer, qUtf8Printable(score_text));

  std::lock_guard<std::mutex> csound_lock(csound_mutex);
  should_play = true;
  start_playing.notify_one();
}

void CsoundData::stop_song() {
  std::unique_lock<std::mutex> csound_lock(csound_mutex);
  should_play = false;
  stop_playing.notify_one();
  while (!ready_to_start) {
    stop_playing.wait(csound_lock);
  }
};

void CsoundData::run_backend() {
  std::unique_lock<std::mutex> csound_lock(csound_mutex);
  while (should_run) {
    {
      start_playing.wait_for(csound_lock, LONG_TIME);
      if (should_play) {
        ready_to_start = false;
        csoundStart(csound_object_pointer);
        while (csoundPerformKsmps(csound_object_pointer) == 0) {
          if (!should_play) {
            break;
          }
          stop_playing.wait_for(csound_lock, SHORT_TIME);
        }
        csoundReset(csound_object_pointer);
        ready_to_start = true;
        stop_playing.notify_one();
      }
    }
    stop_running.wait_for(csound_lock, LONG_TIME);
  }
}

auto csound_thread(void *csound_data_pointer) -> uintptr_t {
  (static_cast<CsoundData *>(csound_data_pointer))->run_backend();
  return 0;
}