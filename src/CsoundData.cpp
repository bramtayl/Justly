#include "CsoundData.h"

#include <QtCore/qtcoreexports.h>  // for qUtf8Printable
#include <bits/chrono.h>           // for milliseconds
#include <csound/csound.h>         // for csoundCompileOrc, csoundCreate
#include <qbytearray.h>            // for QByteArray
#include <qstring.h>               // for QString
#include <QDebug>

#include <thread>  // for sleep_for

const auto SLEEP_TIME = std::chrono::milliseconds(5);

CsoundData::CsoundData()
    : csound_object_pointer(csoundCreate(nullptr)),
      thread_id(csoundCreateThread(csound_thread, (void *)this)){};

CsoundData::~CsoundData() {
  stop_song();
  {
    std::lock_guard<std::mutex> should_stop_running_lock(should_stop_running_mutex);
    should_stop_running = true;
    should_stop_running_condition_variable.notify_one();
  }
  {
    std::unique_lock<std::mutex> is_running_lock(is_running_mutex);
    while (is_running) {
      is_running_condition_variable.wait(is_running_lock);
    }
  }
  {
    std::lock_guard<std::mutex> should_stop_running_lock(should_stop_running_mutex);
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

  {
    std::lock_guard<std::mutex> should_start_playing_lock(should_start_playing_mutex);
    should_start_playing = true;
    should_start_playing_condition_variable.notify_one();
  }
  {
    std::unique_lock<std::mutex> is_playing_lock(is_playing_mutex);
    while (!is_playing) {
      is_playing_condition_variable.wait(is_playing_lock);
    }
  }
}

void CsoundData::stop_song() {
  qInfo("Stopped!");
  {
    std::lock_guard<std::mutex> should_stop_playing_lock(should_stop_playing_mutex);
    should_stop_playing = true;
    should_stop_playing_condition_variable.notify_one();
  }
  {
    std::unique_lock<std::mutex> is_playing_lock(is_playing_mutex);
    while (is_playing) {
      is_playing_condition_variable.wait(is_playing_lock);
    }
  }
  {
    std::lock_guard<std::mutex> should_stop_playing_lock(should_stop_playing_mutex);
    should_stop_playing = false;
  }
};

void CsoundData::run_backend() {
  {
    std::lock_guard<std::mutex> is_running_lock(is_running_mutex);
    is_running = true;
  }
  {
    std::unique_lock<std::mutex> should_stop_running_lock(should_stop_running_mutex);
    while (!should_stop_running) {
      std::unique_lock<std::mutex> should_start_playing_lock(should_start_playing_mutex);
      should_start_playing_condition_variable.wait_for(should_start_playing_lock, std::chrono::milliseconds(100));
      if (should_start_playing) {
        should_start_playing = false;
        {
          std::lock_guard<std::mutex> is_playing_lock(is_playing_mutex);
          is_playing = true;
          is_playing_condition_variable.notify_one();
        }
        csoundStart(csound_object_pointer);
        {
          std::unique_lock<std::mutex> should_stop_playing_lock(should_stop_playing_mutex);
          while (true) {
            should_stop_playing_condition_variable.wait_for(should_stop_playing_lock, std::chrono::nanoseconds(10));
            if (should_stop_playing) {
              should_stop_playing = false;
              break;
            }
            if (csoundPerformKsmps(csound_object_pointer) != 0) {
              break;
            }
          }
        }
        csoundReset(csound_object_pointer);
        {
          std::lock_guard<std::mutex> is_playing_lock(is_playing_mutex);
          is_playing = false;
          is_playing_condition_variable.notify_one();
        }
      }
      should_stop_running_condition_variable.wait_for(should_stop_running_lock, std::chrono::milliseconds(100));
    }
  }
  {
    std::lock_guard<std::mutex> is_running_lock(is_running_mutex);
    is_running = false;
    is_running_condition_variable.notify_one();
  }
}

auto csound_thread(void *csound_data_pointer) -> uintptr_t {
  (static_cast<CsoundData *>(csound_data_pointer))->run_backend();
  return 0;
}