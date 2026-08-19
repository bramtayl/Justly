#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QtAssert>
#include <limits>
#include <string>
#include <zip.h>
#include <zipconf.h>

#include "other/helpers.hpp"

class ZipArchive {
public:
  zip_t *const internal_pointer;

  explicit ZipArchive(const QString &filename)
      : internal_pointer(zip_open(filename.toStdString().c_str(),
                                  ZIP_RDONLY, nullptr)) {}

  ~ZipArchive();

  NO_MOVE_COPY(ZipArchive)
};

// rejects entries whose size libzip couldn't report, or that don't fit in
// the int QByteArray sizes itself with -- casting an oversized size_t down
// to int would both under-allocate the buffer and over-read into it
[[nodiscard]] auto
zip_entry_size_is_safe(const zip_stat_t &entry_stat) -> bool;

// returns an empty QByteArray if the entry is missing or can't be read
[[nodiscard]] auto
read_zip_entry(const ZipArchive &archive,
               const std::string &entry_name) -> QByteArray;
