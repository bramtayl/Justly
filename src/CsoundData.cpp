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
  stop_playing();
  abort();
  csoundJoinThread(thread_id);
  csoundDestroy(csound_object_pointer);
};

void CsoundData::play(const QString &orchestra_text,
                            const QString &score_text) {
  stop_playing();

  std::lock_guard<std::mutex> csound_lock(csound_mutex);
  csoundSetOption(csound_object_pointer, "--output=devaudio");
  csoundCompileOrc(csound_object_pointer, qUtf8Printable(orchestra_text));
  csoundReadScore(csound_object_pointer, qUtf8Printable(score_text));
  
  should_play = true;
  play_signal.notify_one();
}

void CsoundData::stop_playing() {
  std::unique_lock<std::mutex> csound_lock(csound_mutex);
  should_play = false;
  stop_signal.notify_one();
  while (is_playing) {
    ready_signal.wait(csound_lock);
  }
};

void CsoundData::abort() {
  std::lock_guard<std::mutex> csound_lock(csound_mutex);
  should_run = false;
  run_signal.notify_one();
};

void CsoundData::run_backend() {
  std::unique_lock<std::mutex> csound_lock(csound_mutex);
  while (should_run) {
    play_signal.wait_for(csound_lock, LONG_TIME);
    if (should_play) {
      is_playing = true;
      csoundStart(csound_object_pointer);
      while (should_play) {
        if (csoundPerformKsmps(csound_object_pointer) != 0) {
          should_play = false;
          break;
        }
        stop_signal.wait_for(csound_lock, SHORT_TIME);
      }
      csoundReset(csound_object_pointer);
      is_playing = false;
      ready_signal.notify_one();
    }
    run_signal.wait_for(csound_lock, LONG_TIME);
  }
}

auto csound_thread(void *csound_data_pointer) -> uintptr_t {
  (static_cast<CsoundData *>(csound_data_pointer))->run_backend();
  return 0;
}