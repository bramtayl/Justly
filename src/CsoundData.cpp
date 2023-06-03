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
  if (is_running) {
    should_stop_running = true;
    while (is_running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
    }
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
      csoundStart(csound_object_pointer);
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
  return 0;
}