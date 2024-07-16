#pragma once

#include <QColor>
#include <QtCore/QtGlobal>

#if defined(JUSTLY_LIBRARY)
#define JUSTLY_EXPORT Q_DECL_EXPORT
#else
#define JUSTLY_EXPORT Q_DECL_IMPORT
#endif

const auto NOTE_CHORD_COLUMNS = 7;

const auto CHORDS_MIME = "application/json+chords";
const auto NOTES_MIME = "application/json+notes";
const auto INTERVAL_MIME = "application/json+interval";
const auto RATIONAL_MIME = "application/json+rational";
const auto WORDS_MIME = "application/json+words";
const auto INSTRUMENT_MIME = "application/json+instrument";

const auto PERCENT = 100;
const auto MAX_GAIN = 10;
